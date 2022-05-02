#include "CopyBuffer.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "GameFramework/GameState.h"

IMPLEMENT_GLOBAL_SHADER(CopyBufferShader, "/Project/UnrealMasterThesis/CopyBuffer.usf", "MainCompute", SF_Compute);

void CopyBufferShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    TRefCountPtr<FRDGPooledBuffer>* src_buffer,
    TRefCountPtr<FRDGPooledBuffer>* dst_buffer) {

    int N = 210;

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters = graph_builder.AllocParameters<CopyBufferShader::FParameters>();

    FRDGBufferRef dst_ref = graph_builder.RegisterExternalBuffer(*dst_buffer, TEXT("DST_BUFFER"), ERDGBufferFlags::MultiFrame);
    PassParameters->dst = graph_builder.CreateUAV(dst_ref, PF_R32_UINT);

    FRDGBufferRef src_ref = graph_builder.RegisterExternalBuffer(*src_buffer, TEXT("SRC_BUFFER"), ERDGBufferFlags::MultiFrame);
    PassParameters->src = graph_builder.CreateSRV(src_ref, PF_R32_UINT);

    TShaderMapRef<CopyBufferShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("CopyBuffer Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(N, 1, 1));

   
    graph_builder.QueueBufferExtraction(dst_ref, dst_buffer);
    graph_builder.Execute();
}