// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node.h"
#include "K2Node_InputTouch.generated.h"

UCLASS(MinimalAPI)
class UK2Node_InputTouch : public UK2Node
{
	GENERATED_UCLASS_BODY()

	// Prevents actors with lower priority from handling this input
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bConsumeInput:1;

	// Should the binding execute even when the game is paused
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bExecuteWhenPaused:1;

	// Should any bindings to this event in parent classes be removed
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bOverrideParentBinding:1;

	// Begin UObject interface
	virtual void PostLoad() override;
	// End UObject interface

	// Begin UK2Node interface.
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	// End UK2Node interface

	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override{ return TEXT("GraphEditor.TouchEvent_16x"); }
	virtual void GetMenuActions(TArray<UBlueprintNodeSpawner*>& ActionListOut) const override;
	virtual FText GetMenuCategory() const override;
	// End UEdGraphNode interface.

	BLUEPRINTGRAPH_API static UEnum* GetTouchIndexEnum();

	/** Get the 'pressed' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetPressedPin() const;

	/** Get the 'released' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetReleasedPin() const;

	BLUEPRINTGRAPH_API UEdGraphPin* GetLocationPin() const;
	BLUEPRINTGRAPH_API UEdGraphPin* GetFingerIndexPin() const;
	
};
