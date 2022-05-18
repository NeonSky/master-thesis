#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API ButterflyShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(ButterflyShader)
	SHADER_USE_PARAMETER_STRUCT(ButterflyShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, ) 

		SHADER_PARAMETER(int, Stage)
		SHADER_PARAMETER(int, IsVertical)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<FVector4>, ButterflyTexture)

		// Since we ping pong the input texture it probably has to be treaten as RWTexture for reads to be correct (i.e. wait until previous write is done).
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, InputTexture)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, OutputTexture)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	void BuildAndExecuteGraph(FRHICommandListImmediate &RHI_cmd_list, UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* input_output);
};