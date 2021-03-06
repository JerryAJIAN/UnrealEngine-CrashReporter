// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MetalDebugCommandEncoder.h"

NS_ASSUME_NONNULL_BEGIN

@class FMetalShaderPipeline;

@interface FMetalDebugRenderCommandEncoder : FMetalDebugCommandEncoder<MTLRenderCommandEncoder>
{
    @private
#pragma mark - Private Member Variables -
#if METAL_DEBUG_OPTIONS
	FMetalDebugShaderResourceMask ResourceMask[EMetalShaderRenderNum];
    FMetalDebugBufferBindings ShaderBuffers[EMetalShaderRenderNum];
    FMetalDebugTextureBindings ShaderTextures[EMetalShaderRenderNum];
    FMetalDebugSamplerBindings ShaderSamplers[EMetalShaderRenderNum];
#endif
}

/** The wrapped native command-encoder for which we collect debug information. */
@property (readonly, retain) id<MTLRenderCommandEncoder> Inner;
@property (readonly, retain) FMetalDebugCommandBuffer* Buffer;
@property (nonatomic, retain) FMetalShaderPipeline* Pipeline;

/** Initialise the wrapper with the provided command-buffer. */
-(id)initWithEncoder:(id<MTLRenderCommandEncoder>)Encoder andCommandBuffer:(FMetalDebugCommandBuffer*)Buffer;

/** Validates the pipeline/binding state */
-(bool)validateFunctionBindings:(EMetalShaderFrequency)Frequency;
-(void)validate;

@end
NS_ASSUME_NONNULL_END

#if METAL_DEBUG_OPTIONS
#define METAL_SET_RENDER_REFLECTION(Encoder, InPipeline)														\
    if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelFastValidation) \
    {																											\
        ((FMetalDebugRenderCommandEncoder*)Encoder).Pipeline = InPipeline;										\
    }
#else
#define METAL_SET_RENDER_REFLECTION(Encoder, InPipeline)
#endif

