#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API ObstructionShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(ObstructionShader)
	SHADER_USE_PARAMETER_STRUCT(ObstructionShader, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FVector4>, submergedTriangleVertices)
		SHADER_PARAMETER(int, numTriangles)
		SHADER_PARAMETER(int, L)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, obstructionMap_rtt)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, h_rtt)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, v_rtt)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, hPrev_rtt)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, vPrev_rtt)
		SHADER_PARAMETER(float, xPos)
		SHADER_PARAMETER(float, yPos)
		SHADER_PARAMETER(int, boat_dx)
		SHADER_PARAMETER(int, boat_dy)
		SHADER_PARAMETER(float, speedScale)
		SHADER_PARAMETER(int, preFFT)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("N_THREADS_X"), 1);
		OutEnvironment.SetDefine(TEXT("N_THREADS_Y"), 1);
	}

	void BuildAndExecuteGraph(
		FRHICommandListImmediate& RHI_cmd_list,
		TArray<FVector4> submergedTriangleVertices, // TODO: pass as ref instead?
		int numTriangles,
		int L,
		UTextureRenderTarget2D* obstructionMap_rtt,
		UTextureRenderTarget2D* h_rtt,
		UTextureRenderTarget2D* v_rtt,
		UTextureRenderTarget2D* hPrev_rtt,
		UTextureRenderTarget2D* vPrev_rtt,
		float xPos,
		float yPos,
		int boat_dx,
		int boat_dy,
		float speedScale,
		int preFFT
	);
private:
};
