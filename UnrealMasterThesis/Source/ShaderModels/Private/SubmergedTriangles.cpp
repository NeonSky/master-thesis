#include "SubmergedTriangles.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

IMPLEMENT_GLOBAL_SHADER(SubmergedTrianglesShader, "/Project/UnrealMasterThesis/SubmergedTriangles.usf", "MainCompute", SF_Compute);


void SubmergedTrianglesShader::BuildAndExecuteGraph(
        FRHICommandListImmediate &RHI_cmd_list,
        TRefCountPtr<FRDGPooledBuffer>* output_buffer) {
        // FVertexBuffer* output_buffer) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    // Setup input parameters
    FParameters *PassParameters;
    PassParameters = graph_builder.AllocParameters<SubmergedTrianglesShader::FParameters>();

    int N = 140; // 2 times the amount of triangles in the collision mesh

    TArray<GPUSumbergedTriangle> initial_data;
    initial_data.SetNum(N);

    FRDGBufferRef rdg_buffer_ref = CreateStructuredBuffer(
        graph_builder,
        TEXT("SubmergedTriangles"),
        sizeof(GPUSumbergedTriangle),
        N,
        initial_data.GetData(),
        sizeof(GPUSumbergedTriangle) * N,
        ERDGInitialDataFlags::None
    );
    FRDGBufferUAVRef uav_ref = graph_builder.CreateUAV(rdg_buffer_ref, PF_R32_UINT);

    PassParameters->OutputBuffer = uav_ref;

    // Call compute shader
    TShaderMapRef<SubmergedTrianglesShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("SubmergedTriangles Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(N, 1, 1));

    TRefCountPtr<FRDGPooledBuffer> PooledComputeTarget;
    graph_builder.QueueBufferExtraction(rdg_buffer_ref, &PooledComputeTarget);

    graph_builder.Execute();

    *output_buffer = PooledComputeTarget;

	UE_LOG(LogTemp, Warning, TEXT("SUBMERGED DONE"));
}