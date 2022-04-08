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
        TArray<UTextureRenderTarget2D*> other_boat_textures,
        TArray<UTextureRenderTarget2D*> wake_textures,
        TRefCountPtr<FRDGPooledBuffer>* output_buffer,
        TRefCountPtr<FRDGPooledBuffer>* submerged_position_buffer) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    // Setup input parameters
    FParameters *PassParameters;
    PassParameters = graph_builder.AllocParameters<SubmergedTrianglesShader::FParameters>();

    PassParameters->ModelMatrix = collision_mesh->GetActorTransform().ToMatrixWithScale();

    PassParameters->ElevationTexture = register_texture4(graph_builder, elevation_texture, "ElevationRenderTarget");
    PassParameters->BoatTexture      = register_texture4(graph_builder, boat_texture, "BoatRenderTarget");

	// See comment in ElevationSampler.usf
	if (other_boat_textures.Num() > 0) {
        PassParameters->OtherBoatTextures[0] = register_texture4(graph_builder, other_boat_textures[0], "BoatRenderTarget2");
    } else {
		// Assign arbitrary valid texture to prevent crash. It will not be used anyway.
        PassParameters->OtherBoatTextures[0] = register_texture4(graph_builder, boat_texture, "BoatRenderTarget2");
    }

    PassParameters->WakeTextures[0] = register_texture4(graph_builder, wake_textures[0], "WakeRenderTarget");

	// See comment in ElevationSampler.usf
	if (wake_textures.Num() > 1) {
		PassParameters->WakeTextures[1] = register_texture4(graph_builder, wake_textures[1], "WakeRenderTarget2");
	} else {
		// Assign arbitrary valid texture to prevent crash. It will not be used anyway.
		PassParameters->WakeTextures[1] = register_texture4(graph_builder, wake_textures[0], "WakeRenderTarget2");
	}

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

    TArray<FVector4> initial_data2 = {
        FVector4(0.0f, 0.5f, 0, 1),
        FVector4(-1.0f, 0.0f, 0, 1),
        FVector4(0.0f, 0.0f, 0, 1),

        FVector4(1.0f, 1.0f, 0, 1),
        FVector4(0.0f, 0.0f, 0, 1),
        FVector4(1.0f, 0.0f, 0, 1),

        FVector4(0.0f, 0.0f, 0, 1),
        FVector4(-1.0f, -1.0f, 0, 1),
        FVector4(0.0f, -1.0f, 0, 1),

        FVector4(1.0f, 0.0f, 0, 1),
        FVector4(0.0f, -1.0f, 0, 1),
        FVector4(1.0f, -1.0f, 0, 1)
    };
    FRDGBufferDesc desc;
    desc.BytesPerElement = sizeof(FVector4);
    desc.NumElements     = initial_data2.Num();
    desc.UnderlyingType  = FRDGBufferDesc::EUnderlyingType::VertexBuffer;
    desc.Usage           = EBufferUsageFlags(BUF_UnorderedAccess | BUF_ShaderResource | BUF_Dynamic | BUF_VertexBuffer);

    FRDGBufferRef rdg_buffer_ref2 = CreateVertexBuffer(
        graph_builder,
        TEXT("SubmergedPositionBuffer"),
        desc,
        initial_data2.GetData(),
        sizeof(FVector4) * initial_data2.Num(),
        ERDGInitialDataFlags::None
    );
    FRDGBufferUAVRef uav_ref2 = graph_builder.CreateUAV(rdg_buffer_ref2, PF_R32_UINT);
    PassParameters->SubmergedPositionBuffer = uav_ref2;

    // Call compute shader
    TShaderMapRef<SubmergedTrianglesShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("SubmergedTriangles Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(N/2, 1, 1));

    graph_builder.QueueBufferExtraction(rdg_buffer_ref, output_buffer);
    graph_builder.QueueBufferExtraction(rdg_buffer_ref2, submerged_position_buffer);

    graph_builder.Execute();
}