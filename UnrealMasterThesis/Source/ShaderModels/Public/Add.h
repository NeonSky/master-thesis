#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API AddShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(AddShader)
	SHADER_USE_PARAMETER_STRUCT(AddShader, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, term1)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, term2)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, result)

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
		UTextureRenderTarget2D* term1,
		UTextureRenderTarget2D* term2,
		UTextureRenderTarget2D* result
	);
private:
	// LAYOUT_FIELD(FTexture2DRHIRef, heightAddition)
};
