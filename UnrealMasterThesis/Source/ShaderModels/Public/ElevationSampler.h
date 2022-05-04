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

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<FVector4>, elevation_texture)
		SHADER_PARAMETER_RDG_TEXTURE_ARRAY(Texture2D<FVector4>, wake_textures, [2])

		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FVector2D>, input_sample_coordinates)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, output_texture)

		SHADER_PARAMETER_ARRAY(FVector2D, ws_boat_coords, [2])

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
		TArray<UTextureRenderTarget2D*> wake_rtts,
		TArray<FVector2D> ws_boat_coords,
		TArray<FVector2D> input_sample_coordinates,
		TArray<float>* output,
		bool mock_async_readback
	);
};