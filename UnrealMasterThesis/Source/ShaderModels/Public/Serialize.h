#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API SerializeShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(SerializeShader)
	SHADER_USE_PARAMETER_STRUCT(SerializeShader, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, input_rtt)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, serialize_rtt)

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
		UTextureRenderTarget2D* input_rtt,
		UTextureRenderTarget2D* serialize_rtt,
		TArray<FFloat16Color>* data
	);
private:
};
