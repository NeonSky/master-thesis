#include "SubmergedTriangles.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

IMPLEMENT_GLOBAL_SHADER(SubmergedTrianglesShader, "/Project/UnrealMasterThesis/SubmergedTriangles.usf", "MainCompute", SF_Compute);

FRDGTextureRef register_texture4(
	FRDGBuilder& graph_builder,
	UTextureRenderTarget2D* render_target,
	FString name) {

	FRenderTarget* RenderTargetResource = render_target->GetRenderTargetResource();
	FTexture2DRHIRef RenderTargetRHI = RenderTargetResource->GetRenderTargetTexture();

	FSceneRenderTargetItem RenderTargetItem;
	RenderTargetItem.TargetableTexture = RenderTargetRHI;
	RenderTargetItem.ShaderResourceTexture = RenderTargetRHI;
	FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(
		RenderTargetResource->GetSizeXY(),
		RenderTargetRHI->GetFormat(),
		FClearValueBinding::Black,
		TexCreate_None,
		TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV,
		false);
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	GRenderTargetPool.CreateUntrackedElement(RenderTargetDesc, PooledRenderTarget, RenderTargetItem);

	FRDGTextureRef RDG_tex_ref = graph_builder.RegisterExternalTexture(PooledRenderTarget, *name);

	return RDG_tex_ref;
}


void SubmergedTrianglesShader::BuildAndExecuteGraph(
        FRHICommandListImmediate &RHI_cmd_list,
        AStaticMeshActor* collision_mesh,
        UTextureRenderTarget2D* elevation_texture,
        UTextureRenderTarget2D* boat_texture,
        TRefCountPtr<FRDGPooledBuffer>* output_buffer) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    // Setup input parameters
    FParameters *PassParameters;
    PassParameters = graph_builder.AllocParameters<SubmergedTrianglesShader::FParameters>();

    PassParameters->ModelMatrix = collision_mesh->GetActorTransform().ToMatrixWithScale();

    PassParameters->ElevationTexture = register_texture4(graph_builder, elevation_texture, "ElevationRenderTarget");
    PassParameters->BoatTexture      = register_texture4(graph_builder, boat_texture, "BoatRenderTarget");

    FStaticMeshLODResources& mesh_res = collision_mesh->GetStaticMeshComponent()->GetStaticMesh()->RenderData->LODResources[0];
    PassParameters->IndexBuffer = RHI_cmd_list.CreateShaderResourceView(mesh_res.IndexBuffer.IndexBufferRHI);
    PassParameters->PositionBuffer = RHI_cmd_list.CreateShaderResourceView(mesh_res.VertexBuffers.PositionVertexBuffer.VertexBufferRHI, sizeof(float), PF_R32_FLOAT);

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
        FIntVector(N/2, 1, 1));

    TRefCountPtr<FRDGPooledBuffer> PooledComputeTarget;
    graph_builder.QueueBufferExtraction(rdg_buffer_ref, &PooledComputeTarget);

    graph_builder.Execute();

    *output_buffer = PooledComputeTarget;
}