#include "SubmergedTriangles.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

IMPLEMENT_GLOBAL_SHADER(SubmergedTrianglesShader, "/Project/UnrealMasterThesis/SubmergedTriangles.usf", "MainCompute", SF_Compute);

// FRDGBufferRef register_buffer(
// 	FRDGBuilder& graph_builder,
//     FVertexBuffer* buffer,
// 	FString name) {

// 	FRenderTarget* RenderTargetResource = render_target->GetRenderTargetResource();
// 	FTexture2DRHIRef RenderTargetRHI = RenderTargetResource->GetRenderTargetTexture();

// 	// FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(
// 	// 	RenderTargetResource->GetSizeXY(),
// 	// 	RenderTargetRHI->GetFormat(),
// 	// 	FClearValueBinding::Black,
// 	// 	TexCreate_None,
// 	// 	TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV,
// 	// 	false);
//     // NOTE: there is also FRDGBufferDesc::CreateStructuredDesc
//     FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateBufferDesc(4, 5); // https://docs.unrealengine.com/4.26/en-US/API/Runtime/RenderCore/FRDGBufferDesc/CreateBufferDesc/

// 	// TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
//     TRefCountPtr<FPooledRDGBuffer> PooledBuffer;

// 	// FSceneRenderTargetItem RenderTargetItem;
// 	// RenderTargetItem.TargetableTexture = RenderTargetRHI;
// 	// RenderTargetItem.ShaderResourceTexture = RenderTargetRHI;

// 	// GRenderTargetPool.CreateUntrackedElement(RenderTargetDesc, PooledRenderTarget, RenderTargetItem);

//     FRDGBufferRef RDG_ref = graph_builder.RegisterExternalBuffer(PooledBuffer, *name, ERDGBufferFlags::MultiFrame);
// 	// FRDGTextureRef RDG_tex_ref = graph_builder.RegisterExternalTexture(PooledRenderTarget, *name);

//    // https://docs.unrealengine.com/4.27/en-US/API/Runtime/RenderCore/FRDGBuilder/RegisterExternalBuffer/
//    // https://docs.unrealengine.com/4.27/en-US/API/Runtime/RenderCore/CreateStructuredBuffer/

//    // THIS: https://github.com/n-isaeff/UE4_FluidSim/blob/654a4b204be378f00610cbcf702dcb2d8eebf614/Plugins/BeachSimulation/Source/BeachSimulation/Private/CSSimulationComponent.cpp#L108


// 	return RDG_ref;
// }

void SubmergedTrianglesShader::BuildAndExecuteGraph(
        FRHICommandListImmediate &RHI_cmd_list,
        TRefCountPtr<FRDGPooledBuffer>* output_buffer) {
        // FVertexBuffer* output_buffer) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    // Setup input parameters
    FParameters *PassParameters;
    PassParameters = graph_builder.AllocParameters<SubmergedTrianglesShader::FParameters>();

    int N = 5;

    TArray<float> initial_data;
    initial_data.SetNum(N);

    FRDGBufferRef rdg_buffer_ref = CreateStructuredBuffer(
        graph_builder,
        TEXT("SubmergedTriangles"),
        sizeof(float),
        N,
        initial_data.GetData(),
        sizeof(float) * N,
        ERDGInitialDataFlags::None
    );
    FRDGBufferUAVRef uav_ref = graph_builder.CreateUAV(rdg_buffer_ref, PF_R32_UINT);

    PassParameters->OutputBuffer = uav_ref;
    // PassParameters->OutputBuffer = register_buffer(graph_builder, output_buffer->VertexBufferRHI, "output_buffer");

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