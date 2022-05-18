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

		SHADER_PARAMETER_RDG_TEXTURE(RWTexture2D<FVector4>, BoatTexture)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<GPUSumbergedTriangle>, SubmergedTrianglesBuffer)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, obstructionMap_rtt)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, hv_rtt)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, hv_prev_rtt)
		SHADER_PARAMETER(int, preFFT)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("N_TRIANGLES"), 140);
	}

	void BuildAndExecuteGraph(
		FRHICommandListImmediate& RHI_cmd_list,
		UTextureRenderTarget2D* boat_rtt,
		TRefCountPtr<FRDGPooledBuffer> submerged_triangles,
		UTextureRenderTarget2D* obstructionMap_rtt,
		UTextureRenderTarget2D* hv_rtt,
		UTextureRenderTarget2D* hv_prev_rtt,
		int preFFT
	);
private:
};
