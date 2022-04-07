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
		UTextureRenderTarget2D* output
	);

};

class ScreenQuad : public FVertexBuffer {
public:
	void InitRHI() {
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> vertices;
		vertices.SetNumUninitialized(6);

		vertices[0].Position = FVector4(-1, 1, 0, 1);
		vertices[0].UV = FVector2D(0, 0);

		vertices[1].Position = FVector4(1, 1, 0, 1);
		vertices[1].UV = FVector2D(1, 0);

		vertices[2].Position = FVector4(-1, -1, 0, 1);
		vertices[2].UV = FVector2D(0, 1);

		vertices[3].Position = FVector4(1, -1, 0, 1);
		vertices[3].UV = FVector2D(1, 1);

		FRHIResourceCreateInfo CreateInfo(&vertices);
		VertexBufferRHI = RHICreateVertexBuffer(vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};