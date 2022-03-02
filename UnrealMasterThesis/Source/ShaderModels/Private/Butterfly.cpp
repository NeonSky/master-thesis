#include "Butterfly.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

IMPLEMENT_GLOBAL_SHADER(ButterflyShader, "/Project/UnrealMasterThesis/Butterfly.usf", "MainCompute", SF_Compute);

FRDGTextureRef register_texture(
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

void ReadbackRTT(FRHICommandListImmediate &RHI_cmd_list, UTextureRenderTarget2D* rtt) {
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

	for (int i = 0; i < data.Num(); i++) {
		UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, data[i].R.GetFloat(), data[i].G.GetFloat(), data[i].B.GetFloat(), data[i].A.GetFloat());
	}
}

void ButterflyShader::BuildAndExecuteGraph(
  FRHICommandListImmediate &RHI_cmd_list,
  UTextureRenderTarget2D* butterfly,
	UTextureRenderTarget2D* input_output) {

	FRHIResourceCreateInfo CreateInfo;
	uint32 size = butterfly->SizeY;

	// We will use this to build a Rendering Dependency Graph (RDG).
	FRDGBuilder graph_builder(RHI_cmd_list);


	FRDGTextureRef io_tex_ref = register_texture(graph_builder, input_output, "InputOutputRenderTarget");

	FRenderTarget* RenderTargetResource = butterfly->GetRenderTargetResource();
	FTexture2DRHIRef RenderTargetRHI = RenderTargetResource->GetRenderTargetTexture();

	FSceneRenderTargetItem RenderTargetItem;
	RenderTargetItem.TargetableTexture = RenderTargetRHI;
	RenderTargetItem.ShaderResourceTexture = RenderTargetRHI;
	FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(
		RenderTargetResource->GetSizeXY(),
		RenderTargetRHI->GetFormat(),
		FClearValueBinding::Black,
		TexCreate_None,
		TexCreate_RenderTargetable | TexCreate_ShaderResource,
		false);
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	GRenderTargetPool.CreateUntrackedElement(RenderTargetDesc, PooledRenderTarget, RenderTargetItem);

	FRDGTextureRef RDG_tex_ref = graph_builder.RegisterExternalTexture(PooledRenderTarget, TEXT("ButterflyRenderTarget"));


	FRDGTextureDesc tex_desc = FRDGTextureDesc::Create2D(
		FIntPoint(input_output->SizeX, input_output->SizeY),
		PF_FloatRGBA,
		FClearValueBinding(),
		TexCreate_UAV,
		1,
		1
	);
	FRDGTextureRef tex_ref = graph_builder.CreateTexture(tex_desc, TEXT("Compute_Out_Texture"));
	FRDGTextureUAVDesc tex_UAV_desc(tex_ref);

	// ------ Add the compute pass ------
	// Get a reference to our global shader class
	TShaderMapRef<ButterflyShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	int n_stages = butterfly->SizeX;

	FRDGTextureUAVRef input_texture_UAV = graph_builder.CreateUAV(io_tex_ref);
	FRDGTextureUAVRef pingpong_texture_UAV = graph_builder.CreateUAV(tex_UAV_desc);

	int pingpong = 1;
	for (int is_vertical = 0; is_vertical <= 1; is_vertical++) {
		for (int s = 0; s < n_stages; s++) {

			FParameters* PassParameters = graph_builder.AllocParameters<ButterflyShader::FParameters>();
			PassParameters->Stage      = s;
			PassParameters->IsVertical = is_vertical;

			PassParameters->ButterflyTexture = RDG_tex_ref;
			if (pingpong == 1) {
				PassParameters->InputTexture   = input_texture_UAV;
				PassParameters->OutputTexture  = pingpong_texture_UAV;
			} else {
				PassParameters->InputTexture   = pingpong_texture_UAV;
				PassParameters->OutputTexture  = input_texture_UAV;
			}

			FComputeShaderUtils::AddPass(
				graph_builder,
				RDG_EVENT_NAME("Buttterfly FFT Pass"),
				ComputeShader,
				PassParameters,
				FIntVector(size, size, 1)
			);

			pingpong = (pingpong + 1) % 2;
		}
	}

	// ------ Extracting to pooled render target ------
	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget;
	// Copy the result of compute shader from UAV to pooled render target
	graph_builder.QueueTextureExtraction(io_tex_ref, &PooledComputeTarget);

	// Execute the graph
	graph_builder.Execute();

	// Queue the UAV we wrote to for extraction 
	// I.e. copy UAV result on the GPU to our render target (on the CPU?).
	RHI_cmd_list.CopyToResolveTarget(
		PooledComputeTarget.GetReference()->GetRenderTargetItem().TargetableTexture,
		input_output->GetRenderTargetResource()->TextureRHI,
		FResolveParams()
	);
	
	//UE_LOG(LogTemp, Warning, TEXT("FFT OUTPUT START"));
	//ReadbackRTT(RHI_cmd_list, input_output);
	//UE_LOG(LogTemp, Warning, TEXT("FFT OUTPUT END"));

}