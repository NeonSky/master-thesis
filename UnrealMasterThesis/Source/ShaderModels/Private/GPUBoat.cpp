#include "GPUBoat.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"

IMPLEMENT_GLOBAL_SHADER(GPUBoatShader, "/Project/UnrealMasterThesis/GPUBoat.usf", "MainCompute", SF_Compute);

FRDGTextureRef register_texture3(
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

FRDGBufferSRVRef register_buffer(
	FRDGBuilder& graph_builder,
    TRefCountPtr<FRDGPooledBuffer> buffer,
	FString name) {

    FRDGBufferRef RDG_ref = graph_builder.RegisterExternalBuffer(buffer, *name, ERDGBufferFlags::MultiFrame);
	return graph_builder.CreateSRV(RDG_ref);
}

TArray<FFloat16Color> readback_RTT(FRHICommandListImmediate &RHI_cmd_list, UTextureRenderTarget2D* rtt) {
	FRHIResourceCreateInfo CreateInfo;
	FTexture2DRHIRef readback_tex = RHICreateTexture2D(
		rtt->SizeX,
		rtt->SizeY,
		PF_FloatRGBA,
		1,
		1,
		TexCreate_RenderTargetable,
		CreateInfo);

    RHI_cmd_list.CopyToResolveTarget(
        rtt->GetRenderTargetResource()->TextureRHI,
        readback_tex->GetTexture2D(),
        FResolveParams()
    );

	FReadSurfaceDataFlags read_flags(RCM_MinMax);
	read_flags.SetLinearToGamma(false);

	TArray<FFloat16Color> data;
	RHI_cmd_list.ReadSurfaceFloatData(
		readback_tex->GetTexture2D(),
		FIntRect(0, 0, rtt->SizeX, rtt->SizeY),
		data,
		read_flags
	);

    return data;
}

void GPUBoatShader::ResetBoatTexture(
	FRHICommandListImmediate &RHI_cmd_list,
	UTextureRenderTarget2D* input_output) {

	FTexture2DRHIRef tex_ref = input_output->GetRenderTargetResource()->GetTextureRenderTarget2DResource()->GetTextureRHI();

	TArray<FLinearColor> pixel_data = {
		// Current
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), // position
		FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), // orientation
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), // linear velocity
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), // angular velocity
		// Prev
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), // position
		FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), // orientation
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), // linear velocity
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), // angular velocity
	};

	uint32 DestStride = 0;
	FLinearColor* data = (FLinearColor*) RHILockTexture2D(tex_ref, 0, RLM_WriteOnly, DestStride, false);
	FMemory::Memcpy(data, pixel_data.GetData(), sizeof(FLinearColor) * pixel_data.Num());
	RHIUnlockTexture2D(tex_ref, 0, false);
}

void GPUBoatShader::BuildAndExecuteGraph(
    FRHICommandListImmediate &RHI_cmd_list,
    float speed_input,
    FVector2D velocity_input,
    UTextureRenderTarget2D* elevation_texture,
	TRefCountPtr<FRDGPooledBuffer> submerged_triangles_buffer,
	UTextureRenderTarget2D* boat_texture,
	TArray<UTextureRenderTarget2D*> other_boat_textures,
    UTextureRenderTarget2D* readback_rt,
	TArray<FFloat16Color>* readback_target) {

	// We will use this to build a Rendering Dependency Graph (RDG).
	FRDGBuilder graph_builder(RHI_cmd_list);

	FRDGTextureRef io_tex_ref    = register_texture3(graph_builder, boat_texture, "InputOutputRenderTarget");
	FRDGTextureUAVRef io_tex_UAV = graph_builder.CreateUAV(io_tex_ref);

	FRDGTextureRef readback_tex_ref    = register_texture3(graph_builder, readback_rt, "ReadbackRenderTarget");
	FRDGTextureUAVRef readback_tex_UAV = graph_builder.CreateUAV(readback_tex_ref);

    FParameters* PassParameters = graph_builder.AllocParameters<GPUBoatShader::FParameters>();

    PassParameters->SpeedInput    = speed_input;
    PassParameters->VelocityInput = velocity_input;

    PassParameters->ElevationTexture         = register_texture3(graph_builder, elevation_texture, "ElevationRenderTarget");
    PassParameters->SubmergedTrianglesBuffer = register_buffer(graph_builder, submerged_triangles_buffer, "SubmergedTrianglesBuffer");

    PassParameters->BoatTexture = io_tex_UAV;

	// See comment in ElevationSampler.usf
	if (other_boat_textures.Num() > 0) {
        PassParameters->OtherBoatTextures[0] = register_texture3(graph_builder, other_boat_textures[0], "BoatRenderTarget2");
    } else {
		// Assign arbitrary valid texture to prevent crash. It will not be used anyway.
        PassParameters->OtherBoatTextures[0] = register_texture3(graph_builder, boat_texture, "BoatRenderTarget2");
    }

    PassParameters->ReadbackTexture    = readback_tex_UAV;

	TShaderMapRef<GPUBoatShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("GPU Boat Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(1, 1, 1)
    );

	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget;
    if (readback_target) {
        graph_builder.QueueTextureExtraction(readback_tex_ref, &PooledComputeTarget);
    }

	graph_builder.Execute();

    if (readback_target) {
        RHI_cmd_list.CopyToResolveTarget(
            PooledComputeTarget.GetReference()->GetRenderTargetItem().TargetableTexture,
            readback_rt->GetRenderTargetResource()->TextureRHI,
            FResolveParams()
        );
       *readback_target = readback_RTT(RHI_cmd_list, readback_rt);

    }

}