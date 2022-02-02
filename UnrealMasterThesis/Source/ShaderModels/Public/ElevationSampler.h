#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API ElevationSamplerShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(ElevationSamplerShader)
	SHADER_USE_PARAMETER_STRUCT(ElevationSamplerShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, ) 

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<FVector4>, ElevationTexture)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FVector2D>, InputSampleCoordinates)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector4>, OutputBuffer)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, test_output)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("N_THREADS_X"), 1);
	}

	void BuildAndExecuteGraph(
    FRHICommandListImmediate &RHI_cmd_list,
    UTextureRenderTarget2D* elevations,
    TArray<FVector2D> input_sample_coordinates,
		TArray<float>* output
  );
};