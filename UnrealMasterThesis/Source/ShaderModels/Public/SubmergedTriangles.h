#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API SubmergedTrianglesShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(SubmergedTrianglesShader)
	SHADER_USE_PARAMETER_STRUCT(SubmergedTrianglesShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, ) 

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, OutputBuffer)

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
        TRefCountPtr<FRDGPooledBuffer>* output_buffer);

private:

  // https://forums.unrealengine.com/t/assertion-failed-alignedoffset-typelayout-size/243865
  // LAYOUT_FIELD(TRefCountPtr<FRDGPooledBuffer>, m_triangle_data)
};