#include "Obstruction.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256
#define TEMP_TEXTURE_N 256

IMPLEMENT_GLOBAL_SHADER(ObstructionShader, "/Project/UnrealMasterThesis/Obstruction.usf", "eWaveCompute", SF_Compute);

FRDGTextureRef register_texture4_obs(
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

struct CustomUAV {
    FRDGTextureRef ref;
    FRDGTextureUAVRef uav_ref;
};

CustomUAV create_UAV4_obs( // TODO: this and register texture defined in fourier components.
    FRDGBuilder& graph_builder,
    UTextureRenderTarget2D* rtt,
    FString name) {

    FRDGTextureDesc OutTextureDesc = FRDGTextureDesc::Create2D(
        FIntPoint(rtt->SizeX, rtt->SizeY),
        PF_FloatRGBA,
        FClearValueBinding(),
        TexCreate_UAV,
        1,
        1);
    FRDGTextureRef OutTextureRef = graph_builder.CreateTexture(OutTextureDesc, *name);
    FRDGTextureUAVDesc OutTextureUAVDesc(OutTextureRef);

    CustomUAV uav;
    uav.ref = OutTextureRef;
    uav.uav_ref = graph_builder.CreateUAV(OutTextureUAVDesc);

    return uav;
}

void ReadbackRTT3_obs(FRHICommandListImmediate& RHI_cmd_list, UTextureRenderTarget2D* rtt) {
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

void ObstructionShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    TArray<FVector4> submergedTriangleVertices, // TODO: pass as ref instead?
    int numTriangles,
	int L,
    UTextureRenderTarget2D* obstructionMap_rtt,
    UTextureRenderTarget2D* h_rtt,
    UTextureRenderTarget2D* v_rtt,
    UTextureRenderTarget2D* hPrev_rtt,
    UTextureRenderTarget2D* vPrev_rtt,
    float xPos,
    float yPos,
    int boat_dx,
    int boat_dy,
    float speedScale,
    int preFFT) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<ObstructionShader::FParameters>();

    // input_sample_coordinates
    FRDGBufferRef SubmergedTrianglesStructuredBuffer = CreateStructuredBuffer(
        graph_builder,
        TEXT("SubmergedTriangles_StructuredBuffer"),
        sizeof(FVector4), // bytes per element
        numTriangles * 3, // num elements
        submergedTriangleVertices.GetData(),
        sizeof(FVector4) * numTriangles * 3 // initial data size... ? (and no flags parameter)
    );
    FRDGBufferSRVRef SubmergedTrianglesSRV = graph_builder.CreateSRV(SubmergedTrianglesStructuredBuffer, PF_R32_UINT); // PF_R32_FLOAT?
    PassParameters->submergedTriangleVertices = SubmergedTrianglesSRV;

    PassParameters->numTriangles = numTriangles;
	PassParameters->L = L;
    PassParameters->speedScale = speedScale;
    PassParameters->preFFT = preFFT;




	FRDGTextureRef io_tex_ref = register_texture4_obs(graph_builder, obstructionMap_rtt, "InputOutputRenderTarget");
    auto uav = graph_builder.CreateUAV(io_tex_ref);
    FRDGTextureRef io_tex_ref2 = register_texture4_obs(graph_builder, h_rtt, "InputOutputRenderTarget2");
    auto uav2 = graph_builder.CreateUAV(io_tex_ref2);
    FRDGTextureRef io_tex_ref3 = register_texture4_obs(graph_builder, v_rtt, "InputOutputRenderTarget3");
    auto uav3 = graph_builder.CreateUAV(io_tex_ref3);

    FRDGTextureRef io_tex_ref4 = register_texture4_obs(graph_builder, hPrev_rtt, "InputOutputRenderTarget4");
    auto uav4 = graph_builder.CreateUAV(io_tex_ref4);

    FRDGTextureRef io_tex_ref5 = register_texture4_obs(graph_builder, vPrev_rtt, "InputOutputRenderTarget4");
    auto uav5 = graph_builder.CreateUAV(io_tex_ref5);

    PassParameters->obstructionMap_rtt = uav;
    PassParameters->h_rtt = uav2;
    PassParameters->v_rtt = uav3;
    PassParameters->hPrev_rtt = uav4;
    PassParameters->vPrev_rtt = uav5;
    PassParameters->xPos = xPos;
    PassParameters->yPos = yPos;
    PassParameters->boat_dx = boat_dx;
    PassParameters->boat_dy = boat_dy;
    

    TShaderMapRef<ObstructionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("Obstruction Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(TEMP_TEXTURE_N, TEMP_TEXTURE_N, 1)
    );

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget_Obs;
    graph_builder.QueueTextureExtraction(io_tex_ref, &PooledComputeTarget_Obs);

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget2_Obs;
    graph_builder.QueueTextureExtraction(io_tex_ref2, &PooledComputeTarget2_Obs);

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget3_Obs;
    graph_builder.QueueTextureExtraction(io_tex_ref3, &PooledComputeTarget3_Obs);

    graph_builder.Execute();

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget_Obs.GetReference()->GetRenderTargetItem().TargetableTexture,
        obstructionMap_rtt->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget2_Obs.GetReference()->GetRenderTargetItem().TargetableTexture,
        h_rtt->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget3_Obs.GetReference()->GetRenderTargetItem().TargetableTexture,
        v_rtt->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );
   
    //UE_LOG(LogTemp, Warning, TEXT("OBSTRUCTION OUTPUT START"));
    //ReadbackRTT3_obs(RHI_cmd_list, obstructionMap_rtt);
    //UE_LOG(LogTemp, Warning, TEXT("OBSTRUCTION process OUTPUT END"));
}