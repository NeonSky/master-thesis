#include "eWave.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256

IMPLEMENT_GLOBAL_SHADER(eWaveShader, "/Project/UnrealMasterThesis/eWave.usf", "eWaveCompute", SF_Compute);

/*FRDGTextureRef register_texture(
    FRDGBuilder& graph_builder,
    FTexture2DRHIRef rhi_ref,
    FString name) {

    FSceneRenderTargetItem RenderTargetItem;
    RenderTargetItem.TargetableTexture = rhi_ref;
    RenderTargetItem.ShaderResourceTexture = rhi_ref;
    FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(
        rhi_ref->GetSizeXY(),
        rhi_ref->GetFormat(),
        FClearValueBinding::Black,
        TexCreate_None,
        TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV,
        false);
    TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
    GRenderTargetPool.CreateUntrackedElement(RenderTargetDesc, PooledRenderTarget, RenderTargetItem);

    FRDGTextureRef RDG_tex_ref = graph_builder.RegisterExternalTexture(PooledRenderTarget, *name);

    return RDG_tex_ref;
}*/

struct CustomUAV {
    FRDGTextureRef ref;
    FRDGTextureUAVRef uav_ref;
};

CustomUAV create_UAV2( // TODO: this and register texture defined in fourier components.
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

void eWaveShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    float t,
    float L,
    UTextureRenderTarget2D* eWave_h,
    UTextureRenderTarget2D* eWave_hPrev,
    UTextureRenderTarget2D* eWave_v,
    UTextureRenderTarget2D* eWave_vPrev) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<eWaveShader::FParameters>();

    PassParameters->N = NN; // TODO
    PassParameters->L = L;
    PassParameters->t = t;

    CustomUAV uavewave1 = create_UAV2(graph_builder, eWave_h, TEXT("eWave Height"));
    CustomUAV uavewave2 = create_UAV2(graph_builder, eWave_hPrev, TEXT("eWave Height Previous Frame"));
    CustomUAV uavewave3 = create_UAV2(graph_builder, eWave_v, TEXT("eWave Velocity Potential"));
    CustomUAV uavewave4 = create_UAV2(graph_builder, eWave_vPrev, TEXT("eWave Velocity Potential Previous Frame"));

    PassParameters->eWave_h = uavewave1.uav_ref;
    PassParameters->eWave_hPrev = uavewave2.uav_ref;
    PassParameters->eWave_v = uavewave3.uav_ref;
    PassParameters->eWave_vPrev = uavewave4.uav_ref;

    TShaderMapRef<eWaveShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("eWave Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(NN, NN, 1)
    );

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget1_ewave;
    graph_builder.QueueTextureExtraction(uavewave1.ref, &PooledComputeTarget1_ewave);

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget2_ewave;
    graph_builder.QueueTextureExtraction(uavewave2.ref, &PooledComputeTarget2_ewave);

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget3_ewave;
    graph_builder.QueueTextureExtraction(uavewave3.ref, &PooledComputeTarget3_ewave);

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget4_ewave;
    graph_builder.QueueTextureExtraction(uavewave4.ref, &PooledComputeTarget4_ewave);

    graph_builder.Execute();

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget1_ewave.GetReference()->GetRenderTargetItem().TargetableTexture,
        eWave_h->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget2_ewave.GetReference()->GetRenderTargetItem().TargetableTexture,
        eWave_hPrev->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget3_ewave.GetReference()->GetRenderTargetItem().TargetableTexture,
        eWave_v->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget4_ewave.GetReference()->GetRenderTargetItem().TargetableTexture,
        eWave_vPrev->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

    // DEBUG READ-BACK
    // {
    //   FRHIResourceCreateInfo CreateInfo;
    //   FTexture2DRHIRef readback_tex = RHICreateTexture2D(
    //     m_N,
    //     m_N,
    //     PF_FloatRGBA,
    //     1,
    //     1,
    //     TexCreate_RenderTargetable,
    //     CreateInfo);

    //   RHI_cmd_list.CopyToResolveTarget(
    //     tilde_hkt_dy->GetRenderTargetResource()->TextureRHI,
    //     readback_tex->GetTexture2D(),
    //     FResolveParams()
    //   );

    //   UE_LOG(LogTemp, Warning, TEXT("READBACK START"));

    //   FReadSurfaceDataFlags read_flags(RCM_MinMax);
    //   read_flags.SetLinearToGamma(false);

    //   TArray<FFloat16Color> rdata;
    //   RHI_cmd_list.ReadSurfaceFloatData(
    //     readback_tex->GetTexture2D(),
    //     FIntRect(0, 0, m_N, m_N),
    //     rdata,
    //     read_flags
    //   );

    //   UE_LOG(LogTemp, Warning, TEXT("Amount of pixels: %i"), rdata.Num());
    //   for (int i = 0; i < rdata.Num(); i++) {
    //     UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, rdata[i].R.GetFloat(), rdata[i].G.GetFloat(), rdata[i].B.GetFloat(), rdata[i].A.GetFloat());
    //   }
    //   UE_LOG(LogTemp, Warning, TEXT("READBACK END"));
    // }

}