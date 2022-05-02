#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API CopyBufferShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(CopyBufferShader)
	SHADER_USE_PARAMETER_STRUCT(CopyBufferShader, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, src)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, dst)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("N_THREADS_X"), 1);
	}

	void BuildAndExecuteGraph(
		FRHICommandListImmediate& RHI_cmd_list,
		TRefCountPtr<FRDGPooledBuffer>* src_buffer,
		TRefCountPtr<FRDGPooledBuffer>* dst_buffer
	);
};
