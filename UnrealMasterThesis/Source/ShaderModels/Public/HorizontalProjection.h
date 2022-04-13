#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"
#include "Runtime/RenderCore/Public/PixelShaderUtils.h"
#include "Engine/TextureRenderTarget2D.h"

class SHADERMODELS_API HorizontalProjectionVertShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(HorizontalProjectionVertShader)

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

    HorizontalProjectionVertShader() {}
    HorizontalProjectionVertShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}
};

class SHADERMODELS_API HorizontalProjectionFragShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(HorizontalProjectionFragShader)

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

    HorizontalProjectionFragShader() {}
    HorizontalProjectionFragShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}

	void RenderTo(
		FRHICommandListImmediate& RHI_cmd_list,
        FRHIVertexBuffer* submerged_position_buffer,
		UTextureRenderTarget2D* output
	);

};

// A struct is needed. Simply using FVector4 does not work.
struct MyVertex {
	FVector4 Position;
};

class MyVertexDeclaration : public FRenderResource {
public:

	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual ~MyVertexDeclaration() {}

	virtual void InitRHI() {
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(MyVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(MyVertex, Position), VET_Float4, 0, Stride));
		VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() {
		VertexDeclarationRHI.SafeRelease();
	}
};