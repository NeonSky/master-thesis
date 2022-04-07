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
        TRefCountPtr<FRDGPooledBuffer> submerged_position_buffer,
		UTextureRenderTarget2D* output
	);

};

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

class ScreenQuad : public FVertexBuffer {
public:
	void InitRHI() {
		TResourceArray<MyVertex, VERTEXBUFFER_ALIGNMENT> vertices;
		vertices.SetNumUninitialized(6);

		vertices[0].Position = FVector4(-0.5f, 0.5f, 0, 1);
		vertices[1].Position = FVector4(0.5f, 0.5f, 0, 1);
		vertices[2].Position = FVector4(-0.5f, -0.5f, 0, 1);
		vertices[3].Position = FVector4(0.5f, 0.5f, 0, 1);
		vertices[4].Position = FVector4(-0.5f, -0.5f, 0, 1);
		vertices[5].Position = FVector4(0.5f, -0.5f, 0, 1);

		FRHIResourceCreateInfo CreateInfo(&vertices);
		VertexBufferRHI = RHICreateVertexBuffer(vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};