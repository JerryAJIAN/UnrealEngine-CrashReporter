// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Misc/StringAssetReference.h"
#include "Misc/PackageName.h"
#include "InputCoreTypes.h"
#include "EditorStyleSettings.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Model.h"
#include "ISourceControlModule.h"
#include "Settings/ContentBrowserSettings.h"
#include "Settings/DestructableMeshEditorSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Settings/LevelEditorViewportSettings.h"
#include "Settings/EditorProjectSettings.h"
#include "Settings/ClassViewerSettings.h"
#include "Settings/EditorExperimentalSettings.h"
#include "Settings/EditorLoadingSavingSettings.h"
#include "Settings/EditorMiscSettings.h"
#include "Settings/LevelEditorMiscSettings.h"
#include "Settings/ProjectPackagingSettings.h"
#include "EngineGlobals.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "UnrealWidget.h"
#include "EditorModeManager.h"
#include "UnrealEdMisc.h"
#include "CrashReporterSettings.h"
#include "AutoReimport/AutoReimportUtilities.h"
#include "Misc/ConfigCacheIni.h" // for FConfigCacheIni::GetString()
#include "SourceCodeNavigation.h"
#include "Developer/BlueprintProfiler/Public/BlueprintProfilerModule.h"
#include "IProjectManager.h"
#include "ProjectDescriptor.h"
#include "Interfaces/ILauncherWorker.h"

#define LOCTEXT_NAMESPACE "SettingsClasses"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLaunchStartedDelegate, ILauncherProfilePtr, double);

extern LAUNCHERSERVICES_API FOnStageStartedDelegate GLauncherWorker_StageStarted;
extern LAUNCHERSERVICES_API FOnLaunchStartedDelegate GLauncherWorker_LaunchStarted;
extern LAUNCHERSERVICES_API FOnLaunchCanceledDelegate GLauncherWorker_LaunchCanceled;
extern LAUNCHERSERVICES_API FOnLaunchCompletedDelegate GLauncherWorker_LaunchCompleted;

/* UContentBrowserSettings interface
 *****************************************************************************/

UContentBrowserSettings::FSettingChangedEvent UContentBrowserSettings::SettingChangedEvent;

UContentBrowserSettings::UContentBrowserSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
}


void UContentBrowserSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}

/* UClassViewerSettings interface
*****************************************************************************/

UClassViewerSettings::FSettingChangedEvent UClassViewerSettings::SettingChangedEvent;

UClassViewerSettings::UClassViewerSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void UClassViewerSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}

/* UDestructableMeshEditorSettings interface
 *****************************************************************************/

UDestructableMeshEditorSettings::UDestructableMeshEditorSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	AnimPreviewLightingDirection = FRotator(-45.0f, 45.0f, 0);
	AnimPreviewSkyColor = FColor::Blue;
	AnimPreviewFloorColor = FColor(51, 51, 51);
	AnimPreviewSkyBrightness = 0.2f * PI;
	AnimPreviewDirectionalColor = FColor::White;
	AnimPreviewLightBrightness = 1.0f * PI;
}


/* UEditorExperimentalSettings interface
 *****************************************************************************/

UEditorExperimentalSettings::UEditorExperimentalSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
	, bEnableLocalizationDashboard(true)
	, bBlueprintPerformanceAnalysisTools(false)
	, bUseOpenCLForConvexHullDecomp(false)
	, bAllowPotentiallyUnsafePropertyEditing(false)
{
}

void UEditorExperimentalSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	static const FName NAME_EQS = GET_MEMBER_NAME_CHECKED(UEditorExperimentalSettings, bEQSEditor);

	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName(TEXT("ConsoleForGamepadLabels")))
	{
		EKeys::SetConsoleForGamepadLabels(ConsoleForGamepadLabels);
	}
	else if (Name == NAME_EQS)
	{
		if (bEQSEditor)
		{
			FModuleManager::Get().LoadModule(TEXT("EnvironmentQueryEditor"));
		}
	}
	else if (Name == FName(TEXT("bBlueprintPerformanceAnalysisTools")))
	{
		if (!bBlueprintPerformanceAnalysisTools)
		{
			IBlueprintProfilerInterface* ProfilerInterface = FModuleManager::GetModulePtr<IBlueprintProfilerInterface>("BlueprintProfiler");
			if (ProfilerInterface && ProfilerInterface->IsProfilerEnabled())
			{
				// Force Profiler off
				ProfilerInterface->ToggleProfilingCapture();
			}
		}
	}

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}


/* UEditorLoadingSavingSettings interface
 *****************************************************************************/

UEditorLoadingSavingSettings::UEditorLoadingSavingSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
	, bMonitorContentDirectories(true)
	, AutoReimportThreshold(3.f)
	, bAutoCreateAssets(true)
	, bAutoDeleteAssets(true)
	, bDetectChangesOnStartup(true)
	, bDeleteSourceFilesWithAssets(false)
{
	TextDiffToolPath.FilePath = TEXT("P4Merge.exe");

	FAutoReimportDirectoryConfig Default;
	Default.SourceDirectory = TEXT("/Game/");
	AutoReimportDirectorySettings.Add(Default);

	bPromptBeforeAutoImporting = true;
}

// @todo thomass: proper settings support for source control module
void UEditorLoadingSavingSettings::SccHackInitialize()
{
	bSCCUseGlobalSettings = ISourceControlModule::Get().GetUseGlobalSettings();
}

void UEditorLoadingSavingSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Use MemberProperty here so we report the correct member name for nested changes
	const FName Name = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (Name == FName(TEXT("bSCCUseGlobalSettings")))
	{
		// unfortunately we cant use UserSettingChangedEvent here as the source control module cannot depend on the editor
		ISourceControlModule::Get().SetUseGlobalSettings(bSCCUseGlobalSettings);
	}

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}

void UEditorLoadingSavingSettings::PostInitProperties()
{
	if (AutoReimportDirectories_DEPRECATED.Num() != 0)
	{
		AutoReimportDirectorySettings.Empty();
		for (const auto& String : AutoReimportDirectories_DEPRECATED)
		{
			FAutoReimportDirectoryConfig Config;
			Config.SourceDirectory = String;
			AutoReimportDirectorySettings.Add(Config);
		}
		AutoReimportDirectories_DEPRECATED.Empty();
	}
	Super::PostInitProperties();
}

FAutoReimportDirectoryConfig::FParseContext::FParseContext(bool bInEnableLogging)
	: bEnableLogging(bInEnableLogging)
{
	TArray<FString> RootContentPaths;
	FPackageName::QueryRootContentPaths( RootContentPaths );
	for (FString& RootPath : RootContentPaths)
	{
		FString ContentFolder = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(RootPath));
		MountedPaths.Emplace( MoveTemp(ContentFolder), MoveTemp(RootPath) );
	}
}

bool FAutoReimportDirectoryConfig::ParseSourceDirectoryAndMountPoint(FString& SourceDirectory, FString& MountPoint, const FParseContext& InContext)
{
	SourceDirectory.ReplaceInline(TEXT("\\"), TEXT("/"));
	MountPoint.ReplaceInline(TEXT("\\"), TEXT("/"));

	// Check if starts with relative path.
	if (SourceDirectory.StartsWith("../"))
	{
		// Normalize. Interpret setting as a relative path from the Game User directory (Named after the Game)
		SourceDirectory = FPaths::ConvertRelativePathToFull(FPaths::GameUserDir() / SourceDirectory);
	}

	// Check if the source directory is actually a mount point
	const FName SourceDirectoryMountPointName = FPackageName::GetPackageMountPoint(SourceDirectory);
	if (!SourceDirectoryMountPointName.IsNone())
	{
		FString SourceDirectoryMountPoint = SourceDirectoryMountPointName.ToString();
		if (SourceDirectoryMountPoint.Len() + 2 == SourceDirectory.Len())
		{
			// Mount point name + 2 for the directory slashes is the equal, this is exactly a mount point
			MountPoint = SourceDirectory;
			SourceDirectory = FPackageName::LongPackageNameToFilename(MountPoint);
		}
		else
		{
			// Starts off with a mount point (not case sensitive)
			MountPoint = TEXT("/") + SourceDirectoryMountPoint + TEXT("/");
			FString SourceDirectoryLeftChop = SourceDirectory.Left(MountPoint.Len());
			FString SourceDirectoryRightChop = SourceDirectory.RightChop(MountPoint.Len());

			// Resolve mount point on file system (possibly case sensitive, so re-use original source path)
			SourceDirectory = FPaths::ConvertRelativePathToFull(
				FPackageName::LongPackageNameToFilename(SourceDirectoryLeftChop) / SourceDirectoryRightChop);
		}
	}

	if (!SourceDirectory.IsEmpty() && !MountPoint.IsEmpty())
	{
		// We have both a source directory and a mount point. Verify that the source dir exists, and that the mount point is valid.
		if (!IFileManager::Get().DirectoryExists(*SourceDirectory))
		{
			UE_CLOG(InContext.bEnableLogging, LogAutoReimportManager, Warning, TEXT("Unable to watch directory %s as it doesn't exist."), *SourceDirectory);
			return false;
		}

		if (FPackageName::GetPackageMountPoint(MountPoint).IsNone())
		{
			UE_CLOG(InContext.bEnableLogging, LogAutoReimportManager, Warning, TEXT("Unable to setup directory %s to map to %s, as it's not a valid mounted path. Continuing without mounted path (auto reimports will still work, but auto add won't)."), *SourceDirectory, *MountPoint);
			MountPoint = FString();
			return false; // Return false when unable to determine mount point.
		}
	}
	else if(!MountPoint.IsEmpty())
	{
		// We have just a mount point - validate it, and find its source directory
		if (FPackageName::GetPackageMountPoint(MountPoint).IsNone())
		{
			UE_CLOG(InContext.bEnableLogging, LogAutoReimportManager, Warning, TEXT("Unable to setup directory monitor for %s, as it's not a valid mounted path."), *MountPoint);
			return false;
		}

		SourceDirectory = FPackageName::LongPackageNameToFilename(MountPoint);
	}
	else if(!SourceDirectory.IsEmpty())
	{
		// We have just a source directory - verify whether it's a mounted path, and set up the mount point if so
		if (!IFileManager::Get().DirectoryExists(*SourceDirectory))
		{
			UE_CLOG(InContext.bEnableLogging, LogAutoReimportManager, Warning, TEXT("Unable to watch directory %s as it doesn't exist."), *SourceDirectory);
			return false;
		}

		// Set the mounted path if necessary
		auto* Pair = InContext.MountedPaths.FindByPredicate([&](const TPair<FString, FString>& InPair){
			return SourceDirectory.StartsWith(InPair.Key);
		});
		if (Pair)
		{
			// Resolve source directory by replacing mount point with actual path
			MountPoint = Pair->Value / SourceDirectory.RightChop(Pair->Key.Len());
			MountPoint.ReplaceInline(TEXT("\\"), TEXT("/"));
		}
		else
		{
			UE_CLOG(InContext.bEnableLogging, LogAutoReimportManager, Warning, TEXT("Unable to watch directory %s as not associated with mounted path."), *SourceDirectory);
			return false;
		}
	}
	else
	{
		// Don't have any valid settings
		return false;
	}

	return true;
}

/* UEditorMiscSettings interface
 *****************************************************************************/

UEditorMiscSettings::UEditorMiscSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
}


/* ULevelEditorMiscSettings interface
 *****************************************************************************/

ULevelEditorMiscSettings::ULevelEditorMiscSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	bAutoApplyLightingEnable = true;
	SectionName = TEXT("Misc");
	CategoryName = TEXT("LevelEditor");
	EditorScreenshotSaveDirectory.Path = FPaths::ScreenShotDir();
}


void ULevelEditorMiscSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (Name == FName(TEXT("bNavigationAutoUpdate")))
	{
		FWorldContext &EditorContext = GEditor->GetEditorWorldContext();
		UNavigationSystem::SetNavigationAutoUpdateEnabled(bNavigationAutoUpdate, EditorContext.World()->GetNavigationSystem());
	}

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}
}


/* ULevelEditorPlaySettings interface
 *****************************************************************************/

ULevelEditorPlaySettings::ULevelEditorPlaySettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	ClientWindowWidth = 640;
	ClientWindowHeight = 480;
	PlayNumberOfClients = 1;
	PlayNetDedicated = false;
	RunUnderOneProcess = true;
	RouteGamepadToSecondWindow = false;
	AutoConnectToServer = true;
	BuildGameBeforeLaunch = EPlayOnBuildMode::PlayOnBuild_Default;
	LaunchConfiguration = EPlayOnLaunchConfiguration::LaunchConfig_Default;
	bAutoCompileBlueprintsOnLaunch = true;
	CenterNewWindow = true;
	CenterStandaloneWindow = true;

	bBindSequencerToPIE = false;
	bBindSequencerToSimulate = true;
	EnablePIEEnterAndExitSounds = false;
}

void ULevelEditorPlaySettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (BuildGameBeforeLaunch != EPlayOnBuildMode::PlayOnBuild_Always && !FSourceCodeNavigation::IsCompilerAvailable())
	{
		BuildGameBeforeLaunch = EPlayOnBuildMode::PlayOnBuild_Never;
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

/* ULevelEditorViewportSettings interface
 *****************************************************************************/

ULevelEditorViewportSettings::ULevelEditorViewportSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	bLevelStreamingVolumePrevis = false;
	BillboardScale = 1.0f;
	TransformWidgetSizeAdjustment = 0.0f;
	MeasuringToolUnits = MeasureUnits_Centimeters;

	// Set a default preview mesh
	PreviewMeshes.Add(FStringAssetReference("/Engine/EditorMeshes/ColorCalibrator/SM_ColorCalibrator.SM_ColorCalibrator"));
}

void ULevelEditorViewportSettings::PostInitProperties()
{
	Super::PostInitProperties();
	UBillboardComponent::SetEditorScale(BillboardScale);
	UArrowComponent::SetEditorScale(BillboardScale);
}

void ULevelEditorViewportSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, bAllowTranslateRotateZWidget))
	{
		if (bAllowTranslateRotateZWidget)
		{
			GLevelEditorModeTools().SetWidgetMode(FWidget::WM_TranslateRotateZ);
		}
		else if (GLevelEditorModeTools().GetWidgetMode() == FWidget::WM_TranslateRotateZ)
		{
			GLevelEditorModeTools().SetWidgetMode(FWidget::WM_Translate);
		}
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, bHighlightWithBrackets))
	{
		GEngine->SetSelectedMaterialColor(bHighlightWithBrackets
			? FLinearColor::Black
			: GetDefault<UEditorStyleSettings>()->SelectionColor);
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, HoverHighlightIntensity))
	{
		GEngine->HoverHighlightIntensity = HoverHighlightIntensity;
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, SelectionHighlightIntensity))
	{
		GEngine->SelectionHighlightIntensity = SelectionHighlightIntensity;
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, BSPSelectionHighlightIntensity))
	{
		GEngine->BSPSelectionHighlightIntensity = BSPSelectionHighlightIntensity;
	}
	else if ((Name == FName(TEXT("UserDefinedPosGridSizes"))) || (Name == FName(TEXT("UserDefinedRotGridSizes"))) || (Name == FName(TEXT("ScalingGridSizes"))) || (Name == FName(TEXT("GridIntervals")))) //@TODO: This should use GET_MEMBER_NAME_CHECKED
	{
		const float MinGridSize = (Name == FName(TEXT("GridIntervals"))) ? 4.0f : 0.0001f; //@TODO: This should use GET_MEMBER_NAME_CHECKED
		TArray<float>* ArrayRef = nullptr;
		int32* IndexRef = nullptr;

		if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, ScalingGridSizes))
		{
			ArrayRef = &(ScalingGridSizes);
			IndexRef = &(CurrentScalingGridSize);
		}

		if (ArrayRef && IndexRef)
		{
			// Don't allow an empty array of grid sizes
			if (ArrayRef->Num() == 0)
			{
				ArrayRef->Add(MinGridSize);
			}

			// Don't allow negative numbers
			for (int32 SizeIdx = 0; SizeIdx < ArrayRef->Num(); ++SizeIdx)
			{
				if ((*ArrayRef)[SizeIdx] < MinGridSize)
				{
					(*ArrayRef)[SizeIdx] = MinGridSize;
				}
			}
		}
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, bUsePowerOf2SnapSize))
	{
		const float BSPSnapSize = bUsePowerOf2SnapSize ? 128.0f : 100.0f;
		UModel::SetGlobalBSPTexelScale(BSPSnapSize);
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, BillboardScale))
	{
		UBillboardComponent::SetEditorScale(BillboardScale);
		UArrowComponent::SetEditorScale(BillboardScale);
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ULevelEditorViewportSettings, bEnableLayerSnap))
	{
		ULevelEditor2DSettings* Settings2D = GetMutableDefault<ULevelEditor2DSettings>();
		if (bEnableLayerSnap && !Settings2D->bEnableSnapLayers)
		{
			Settings2D->bEnableSnapLayers = true;
		}
	}

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	GEditor->RedrawAllViewports();

	SettingChangedEvent.Broadcast(Name);
}

/* FBlueprintNativizationMethodGlobals
 *****************************************************************************/

void UProjectPackagingSettings_EnableBlueprintNativizationProjectSettings(bool bEnable);

namespace FBlueprintNativizationMethodGlobals
{
	FCriticalSection Mutex;
	int32 DisabledCount = 0;
	EProjectPackagingBlueprintNativizationMethod CachedValue = EProjectPackagingBlueprintNativizationMethod::Disabled;

	FDelegateHandle LauncherWorker_OnStageStarted;
	FDelegateHandle LauncherWorker_OnLaunchStarted;
	FDelegateHandle LauncherWorker_OnLaunchCanceled;
	FDelegateHandle LauncherWorker_OnLaunchCompleted;

	void OnLauncherWorker_LaunchStarted(ILauncherProfilePtr InProfile, double InStartTime)
	{
		UProjectPackagingSettings_EnableBlueprintNativizationProjectSettings(false);
	}

	void OnLauncherWorker_StageStarted(const FString& InStageName)
	{
		if (InStageName.Equals(TEXT("Run Task")))
		{
			UProjectPackagingSettings_EnableBlueprintNativizationProjectSettings(true);
		}
	}

	void OnLauncherWorker_LaunchCanceled(double InDuration)
	{
		UProjectPackagingSettings_EnableBlueprintNativizationProjectSettings(true);
	}

	void OnLauncherWorker_LaunchCompleted(bool bSucceeded, double InDuration, int32 InResultCode)
	{
		UProjectPackagingSettings_EnableBlueprintNativizationProjectSettings(true);
	}
}

/* UProjectPackagingSettings interface
*****************************************************************************/

UProjectPackagingSettings::UProjectPackagingSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	bWarnIfPackagedWithoutNativizationFlag = true;
}


void UProjectPackagingSettings::PostInitProperties()
{
	// Migrate from deprecated Blueprint nativization packaging flags.
	// Note: This assumes that LoadConfig() has been called before getting here.
	FString NewValue;
	const FString ConfigFileName = UProjectPackagingSettings::StaticClass()->GetConfigName();
	const FString ClassSectionName = UProjectPackagingSettings::StaticClass()->GetPathName();
	const bool bIgnoreOldFlags = GConfig->GetString(*ClassSectionName, GET_MEMBER_NAME_STRING_CHECKED(UProjectPackagingSettings, BlueprintNativizationMethod), NewValue, ConfigFileName);
	if (!bIgnoreOldFlags && bNativizeBlueprintAssets_DEPRECATED)
	{
		BlueprintNativizationMethod = bNativizeOnlySelectedBlueprints_DEPRECATED ? EProjectPackagingBlueprintNativizationMethod::Exclusive : EProjectPackagingBlueprintNativizationMethod::Inclusive;
	}

	// Reset deprecated settings to defaults.
	bNativizeBlueprintAssets_DEPRECATED = false;
	bNativizeOnlySelectedBlueprints_DEPRECATED = false;

	// Cache the current set of Blueprint assets selected for nativization.
	CachedNativizeBlueprintAssets = NativizeBlueprintAssets;

	Super::PostInitProperties();
}


void UProjectPackagingSettings::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.MemberProperty != nullptr)
		? PropertyChangedEvent.MemberProperty->GetFName()
		: NAME_None;

	if (Name == FName((TEXT("DirectoriesToAlwaysCook"))))
	{
		// fix up paths
		for(int32 PathIndex = 0; PathIndex < DirectoriesToAlwaysCook.Num(); PathIndex++)
		{
			FString Path = DirectoriesToAlwaysCook[PathIndex].Path;
			FPaths::MakePathRelativeTo(Path, FPlatformProcess::BaseDir());
			DirectoriesToAlwaysCook[PathIndex].Path = Path;
		}
	}
	else if (Name == FName((TEXT("StagingDirectory"))))
	{
		// fix up path
		FString Path = StagingDirectory.Path;
		FPaths::MakePathRelativeTo(Path, FPlatformProcess::BaseDir());
		StagingDirectory.Path = Path;
	}
	else if (Name == FName(TEXT("ForDistribution")) || Name == FName(TEXT("BuildConfiguration")))
	{
		if (ForDistribution)
		{
			BuildConfiguration = EProjectPackagingBuildConfigurations::PPBC_Shipping;
		}
	}
	else if (Name == FName(TEXT("bGenerateChunks")))
	{
		if (bGenerateChunks)
		{
			UsePakFile = true;
		}
	}
	else if (Name == FName(TEXT("UsePakFile")))
	{
		if (!UsePakFile)
		{
			bGenerateChunks = false;
			bBuildHttpChunkInstallData = false;
		}
	}
	else if (Name == FName(TEXT("bBuildHTTPChunkInstallData")))
	{
		if (bBuildHttpChunkInstallData)
		{
			UsePakFile = true;
			bGenerateChunks = true;
			//Ensure data is something valid
			if (HttpChunkInstallDataDirectory.Path.IsEmpty())
			{
				auto CloudInstallDir = FPaths::ConvertRelativePathToFull(FPaths::GetPath(FPaths::GetProjectFilePath())) / TEXT("ChunkInstall");
				HttpChunkInstallDataDirectory.Path = CloudInstallDir;
			}
			if (HttpChunkInstallDataVersion.IsEmpty())
			{
				HttpChunkInstallDataVersion = TEXT("release1");
			}
		}
	}
	else if (Name == FName((TEXT("ApplocalPrerequisitesDirectory"))))
	{
		// If a variable is already in use, assume the user knows what they are doing and don't modify the path
		if(!ApplocalPrerequisitesDirectory.Path.Contains("$("))
		{
			// Try making the path local to either project or engine directories.
			FString EngineRootedPath = ApplocalPrerequisitesDirectory.Path;
			FString EnginePath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(FPaths::EngineDir())) + "/";
			FPaths::MakePathRelativeTo(EngineRootedPath, *EnginePath);
			if (FPaths::IsRelative(EngineRootedPath))
			{
				ApplocalPrerequisitesDirectory.Path = "$(EngineDir)/" + EngineRootedPath;
				return;
			}

			FString ProjectRootedPath = ApplocalPrerequisitesDirectory.Path;
			FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(FPaths::GetProjectFilePath())) + "/";
			FPaths::MakePathRelativeTo(ProjectRootedPath, *ProjectPath);
			if (FPaths::IsRelative(EngineRootedPath))
			{
				ApplocalPrerequisitesDirectory.Path = "$(ProjectDir)/" + ProjectRootedPath;
				return;
			}
		}
	}
	else if (Name == FName((TEXT("BlueprintNativizationMethod"))))
	{
		// This exists to guard against the remote possibility of parallel execution of a Project Settings UI edit and a Project Launcher worker thread, for example.
		FScopeLock ScopeLock(&FBlueprintNativizationMethodGlobals::Mutex);

		// Keep the current Blueprint nativization method setting locked while it is disabled.
		if (FBlueprintNativizationMethodGlobals::DisabledCount > 0)
		{
			// Defer the current project file update until we restore Blueprint nativization.
			FBlueprintNativizationMethodGlobals::CachedValue = BlueprintNativizationMethod;
			BlueprintNativizationMethod = EProjectPackagingBlueprintNativizationMethod::Disabled;
		}
		else
		{
			IProjectManager& ProjManager = IProjectManager::Get();
			{
				// NOTE: these are hardcoded to match the path constructed by AddBlueprintPluginPathArgument() on CookCommand.Automation.cs, and the defaults in 
				//       FBlueprintNativeCodeGenPaths::GetDefaultCodeGenPaths(); if you alter this (or either of those) then you need to update the others
				const FString NativizedPluginDir = TEXT("./Intermediate/Plugins");
				const FString NativizedPluginName = TEXT("NativizedAssets");
				const FString FullPluginPath = FPaths::ConvertRelativePathToFull(FPaths::ConvertRelativePathToFull(FPaths::GameDir()), NativizedPluginDir);

				if (BlueprintNativizationMethod == EProjectPackagingBlueprintNativizationMethod::Disabled)
				{

					ProjManager.UpdateAdditionalPluginDirectory(FullPluginPath, /*bAddOrRemove =*/false);
					FText PluginDisableFailure;
					ProjManager.SetPluginEnabled(NativizedPluginName, /*bEnabled =*/false, PluginDisableFailure);
				}
				else
				{
					ProjManager.UpdateAdditionalPluginDirectory(FullPluginPath, /*bAddOrRemove =*/true);
					// plugin is enabled by default, so let's remove it from the uproject list entirely (else it causes problem in the packaged project)
					// SetPluginEnabled() will only remove it if the plugin exists (it may not yet), so we rely on this explicit removal
					FText PluginEnableFailure;
					ProjManager.RemovePluginReference(NativizedPluginName, PluginEnableFailure);
				}

				FText SaveFailure;
				ProjManager.SaveCurrentProjectToDisk(SaveFailure);
			}
		}
	}
	else if (Name == FName((TEXT("NativizeBlueprintAssets"))))
	{
		int32 AssetIndex;
		auto OnSelectBlueprintForExclusiveNativizationLambda = [](const FString& PackageName, bool bSelect)
		{
			if (!PackageName.IsEmpty())
			{
				// This should only apply to loaded packages. Any unloaded packages defer setting the transient flag to when they're loaded.
				if (UPackage* Package = FindPackage(nullptr, *PackageName))
				{
					// Find the Blueprint asset within the package.
					if (UBlueprint* Blueprint = FindObject<UBlueprint>(Package, *FPaths::GetBaseFilename(PackageName)))
					{
						// We're toggling the transient flag on or off.
						if ((Blueprint->NativizationFlag == EBlueprintNativizationFlag::ExplicitlyEnabled) != bSelect)
						{
							Blueprint->NativizationFlag = bSelect ? EBlueprintNativizationFlag::ExplicitlyEnabled : EBlueprintNativizationFlag::Disabled;
						}
					}
				}
			}
		};

		if (NativizeBlueprintAssets.Num() > 0)
		{
			for (AssetIndex = 0; AssetIndex < NativizeBlueprintAssets.Num(); ++AssetIndex)
			{
				const FString& PackageName = NativizeBlueprintAssets[AssetIndex].FilePath;
				if (AssetIndex >= CachedNativizeBlueprintAssets.Num())
				{
					// A new entry was added; toggle the exclusive flag on the corresponding Blueprint asset (if loaded).
					OnSelectBlueprintForExclusiveNativizationLambda(PackageName, true);

					// Add an entry to the end of the cached list.
					CachedNativizeBlueprintAssets.Add(NativizeBlueprintAssets[AssetIndex]);
				}
				else if (!PackageName.Equals(CachedNativizeBlueprintAssets[AssetIndex].FilePath))
				{
					if (NativizeBlueprintAssets.Num() < CachedNativizeBlueprintAssets.Num())
					{
						// An entry was removed; toggle the exclusive flag on the corresponding Blueprint asset (if loaded).
						OnSelectBlueprintForExclusiveNativizationLambda(CachedNativizeBlueprintAssets[AssetIndex].FilePath, false);

						// Remove this entry from the cached list.
						CachedNativizeBlueprintAssets.RemoveAt(AssetIndex);
					}
					else if(NativizeBlueprintAssets.Num() > CachedNativizeBlueprintAssets.Num())
					{
						// A new entry was inserted; toggle the exclusive flag on the corresponding Blueprint asset (if loaded).
						OnSelectBlueprintForExclusiveNativizationLambda(PackageName, true);

						// Insert the new entry into the cached list.
						CachedNativizeBlueprintAssets.Insert(NativizeBlueprintAssets[AssetIndex], AssetIndex);
					}
					else
					{
						// An entry was changed; toggle the exclusive flag on the corresponding Blueprint assets (if loaded).
						OnSelectBlueprintForExclusiveNativizationLambda(CachedNativizeBlueprintAssets[AssetIndex].FilePath, false);
						OnSelectBlueprintForExclusiveNativizationLambda(PackageName, true);

						// Update the cached entry.
						CachedNativizeBlueprintAssets[AssetIndex].FilePath = PackageName;
					}
				}
			}

			if (CachedNativizeBlueprintAssets.Num() > NativizeBlueprintAssets.Num())
			{
				// Removed entries at the end of the list; toggle the exclusive flag on the corresponding Blueprint asset(s) (if loaded).
				for (AssetIndex = NativizeBlueprintAssets.Num(); AssetIndex < CachedNativizeBlueprintAssets.Num(); ++AssetIndex)
				{
					OnSelectBlueprintForExclusiveNativizationLambda(CachedNativizeBlueprintAssets[AssetIndex].FilePath, false);
				}

				// Remove entries from the end of the cached list.
				CachedNativizeBlueprintAssets.RemoveAt(NativizeBlueprintAssets.Num(), CachedNativizeBlueprintAssets.Num() - NativizeBlueprintAssets.Num());
			}
		}
		else if(CachedNativizeBlueprintAssets.Num() > 0)
		{
			// Removed all entries; toggle the exclusive flag on the corresponding Blueprint asset(s) (if loaded).
			for (AssetIndex = 0; AssetIndex < CachedNativizeBlueprintAssets.Num(); ++AssetIndex)
			{
				OnSelectBlueprintForExclusiveNativizationLambda(CachedNativizeBlueprintAssets[AssetIndex].FilePath, false);
			}

			// Clear the cached list.
			CachedNativizeBlueprintAssets.Empty();
		}
	}
}

bool UProjectPackagingSettings::CanEditChange( const UProperty* InProperty ) const
{
	if (InProperty->GetFName() == FName(TEXT("BuildConfiguration")) && ForDistribution)
	{
		return false;
	}
	else if (InProperty->GetFName() == FName(TEXT("BlueprintNativizationMethod")))
	{
		return FBlueprintNativizationMethodGlobals::DisabledCount == 0;
	}
	else if (InProperty->GetFName() == FName(TEXT("NativizeBlueprintAssets")))
	{
		return BlueprintNativizationMethod == EProjectPackagingBlueprintNativizationMethod::Exclusive;
	}

	return Super::CanEditChange(InProperty);
}

bool UProjectPackagingSettings::AddBlueprintAssetToNativizationList(const class UBlueprint* InBlueprint)
{
	if (InBlueprint)
	{
		const FString PackageName = InBlueprint->GetOutermost()->GetName();

		// Make sure it's not already in the exclusive list. This can happen if the user previously added this asset in the Project Settings editor.
		const bool bFound = IsBlueprintAssetInNativizationList(InBlueprint);
		if (!bFound)
		{
			// Add this Blueprint asset to the exclusive list.
			FFilePath FileInfo;
			FileInfo.FilePath = PackageName;
			NativizeBlueprintAssets.Add(FileInfo);

			// Also add it to the mirrored list for tracking edits.
			CachedNativizeBlueprintAssets.Add(FileInfo);

			return true;
		}
	}

	return false;
}

bool UProjectPackagingSettings::RemoveBlueprintAssetFromNativizationList(const class UBlueprint* InBlueprint)
{
	if (InBlueprint)
	{
		const FString PackageName = InBlueprint->GetOutermost()->GetName();

		int32 AssetIndex = FindBlueprintInNativizationList(InBlueprint);
		if (AssetIndex >= 0)
		{
			// Note: Intentionally not using RemoveAtSwap() here, so that the order is preserved.
			NativizeBlueprintAssets.RemoveAt(AssetIndex);

			// Also remove it from the mirrored list (for tracking edits).
			CachedNativizeBlueprintAssets.RemoveAt(AssetIndex);

			return true;
		}
	}

	return false;
}

int32 UProjectPackagingSettings::FindBlueprintInNativizationList(const UBlueprint* InBlueprint) const
{
	int32 ListIndex = INDEX_NONE;
	if (InBlueprint)
	{
		const FString PackageName = InBlueprint->GetOutermost()->GetName();

		for (int32 AssetIndex = 0; AssetIndex < NativizeBlueprintAssets.Num(); ++AssetIndex)
		{
			if (NativizeBlueprintAssets[AssetIndex].FilePath.Equals(PackageName, ESearchCase::IgnoreCase))
			{
				ListIndex = AssetIndex;
				break;
			}
		}
	}
	return ListIndex;
}

void UProjectPackagingSettings_StartBlueprintNativizationProjectSettings()
{
	FBlueprintNativizationMethodGlobals::LauncherWorker_OnStageStarted = GLauncherWorker_StageStarted.AddStatic(&FBlueprintNativizationMethodGlobals::OnLauncherWorker_StageStarted);
	FBlueprintNativizationMethodGlobals::LauncherWorker_OnLaunchStarted = GLauncherWorker_LaunchStarted.AddStatic(&FBlueprintNativizationMethodGlobals::OnLauncherWorker_LaunchStarted);
	FBlueprintNativizationMethodGlobals::LauncherWorker_OnLaunchCanceled = GLauncherWorker_LaunchCanceled.AddStatic(&FBlueprintNativizationMethodGlobals::OnLauncherWorker_LaunchCanceled);
	FBlueprintNativizationMethodGlobals::LauncherWorker_OnLaunchCompleted = GLauncherWorker_LaunchCompleted.AddStatic(&FBlueprintNativizationMethodGlobals::OnLauncherWorker_LaunchCompleted);
}

void UProjectPackagingSettings_ShutdownBlueprintNativizationProjectSettings()
{
	GLauncherWorker_StageStarted.Remove(FBlueprintNativizationMethodGlobals::LauncherWorker_OnStageStarted);
	GLauncherWorker_LaunchStarted.Remove(FBlueprintNativizationMethodGlobals::LauncherWorker_OnLaunchStarted);
	GLauncherWorker_LaunchCanceled.Remove(FBlueprintNativizationMethodGlobals::LauncherWorker_OnLaunchCanceled);
	GLauncherWorker_LaunchCompleted.Remove(FBlueprintNativizationMethodGlobals::LauncherWorker_OnLaunchCompleted);
}

void UProjectPackagingSettings_EnableBlueprintNativizationProjectSettings(bool bEnable)
{
	// Guard against parallel execution by multiple Project Launcher worker threads.
	FScopeLock ScopeLock(&FBlueprintNativizationMethodGlobals::Mutex);

	const bool bIsDisabled = (FBlueprintNativizationMethodGlobals::DisabledCount > 0);
	UProjectPackagingSettings* Settings = GetMutableDefault<UProjectPackagingSettings>();

	auto SetBlueprintNativizationMethodLambda = [Settings](EProjectPackagingBlueprintNativizationMethod InMethod)
	{
		// Update the actual setting.
		Settings->BlueprintNativizationMethod = InMethod;

		// Must call this in order to update the current project file to reflect the setting change.
		static UProperty* Property = FindFieldChecked<UProperty>(UProjectPackagingSettings::StaticClass(), GET_MEMBER_NAME_CHECKED(UProjectPackagingSettings, BlueprintNativizationMethod));
		FPropertyChangedEvent PropertyChangedEvent(Property, EPropertyChangeType::ValueSet);
		Settings->PostEditChangeProperty(PropertyChangedEvent);
	};

	if (bEnable)
	{
		if (bIsDisabled)
		{
			// Decrement the call stack. Reset only when we hit zero.
			if (--FBlueprintNativizationMethodGlobals::DisabledCount == 0)
			{
				// Apply previous setting.
				SetBlueprintNativizationMethodLambda(FBlueprintNativizationMethodGlobals::CachedValue);

				// Reset the cached value.
				FBlueprintNativizationMethodGlobals::CachedValue = EProjectPackagingBlueprintNativizationMethod::Disabled;
			}
		}
	}
	else
	{
		if (!bIsDisabled)
		{
			// Cache the current setting if we haven't already.
			FBlueprintNativizationMethodGlobals::CachedValue = Settings->BlueprintNativizationMethod;

			// Switch the current nativization method to be disabled.
			SetBlueprintNativizationMethodLambda(EProjectPackagingBlueprintNativizationMethod::Disabled);
		}

		// Increment the call stack.
		++FBlueprintNativizationMethodGlobals::DisabledCount;
	}
}

/* UCrashReporterSettings interface
*****************************************************************************/
UCrashReporterSettings::UCrashReporterSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#undef LOCTEXT_NAMESPACE
