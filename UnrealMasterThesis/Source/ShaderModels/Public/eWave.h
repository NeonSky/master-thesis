#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API eWaveShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(eWaveShader)
	SHADER_USE_PARAMETER_STRUCT(eWaveShader, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER(float, dt)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, eWave_hv)

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
		float dt,
		UTextureRenderTarget2D* eWave_hv
	);
private:
};
