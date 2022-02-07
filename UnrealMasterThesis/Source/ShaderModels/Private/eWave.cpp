#include "eWave.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 8
#define TEMP_TEXTURE_N 8

IMPLEMENT_GLOBAL_SHADER(eWaveShader, "/Project/UnrealMasterThesis/eWave.usf", "eWaveCompute", SF_Compute);

FRDGTextureRef register_texture5(
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

   // CustomUAV uavewave1 = create_UAV2(graph_builder, eWave_h, TEXT("eWave Height"));
   // CustomUAV uavewave2 = create_UAV2(graph_builder, eWave_hPrev, TEXT("eWave Height Previous Frame"));
   // CustomUAV uavewave3 = create_UAV2(graph_builder, eWave_v, TEXT("eWave Velocity Potential"));
   // CustomUAV uavewave4 = create_UAV2(graph_builder, eWave_vPrev, TEXT("eWave Velocity Potential Previous Frame"));

    FRDGTextureRef io_tex_ref1 = register_texture5(graph_builder, eWave_h, "eWave Height");
    FRDGTextureRef io_tex_ref2 = register_texture5(graph_builder, eWave_hPrev, "eWave Height Previous Frame");
    FRDGTextureRef io_tex_ref3 = register_texture5(graph_builder, eWave_v, "eWave Velocity Potential");
    FRDGTextureRef io_tex_ref4 = register_texture5(graph_builder, eWave_vPrev, "eWave Velocity Potential Previous Frame");
    auto uav1 = graph_builder.CreateUAV(io_tex_ref1);
    auto uav2 = graph_builder.CreateUAV(io_tex_ref2);
    auto uav3 = graph_builder.CreateUAV(io_tex_ref3);
    auto uav4 = graph_builder.CreateUAV(io_tex_ref4);

    PassParameters->eWave_h = uav1;
    PassParameters->eWave_hPrev = uav2;
    PassParameters->eWave_v = uav3;
    PassParameters->eWave_vPrev = uav4;

    TShaderMapRef<eWaveShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("eWave Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(NN, NN, 1)
    );

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget1_ewave;
    graph_builder.QueueTextureExtraction(io_tex_ref1, &PooledComputeTarget1_ewave);
    /*TRefCountPtr<IPooledRenderTarget> PooledComputeTarget2_ewave;
    graph_builder.QueueTextureExtraction(io_tex_ref2, &PooledComputeTarget2_ewave);
    
    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget3_ewave;
    graph_builder.QueueTextureExtraction(io_tex_ref3, &PooledComputeTarget3_ewave);
    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget4_ewave;
    graph_builder.QueueTextureExtraction(io_tex_ref4, &PooledComputeTarget4_ewave);*/

    graph_builder.Execute();

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget1_ewave.GetReference()->GetRenderTargetItem().TargetableTexture,
        eWave_h->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

    /*RHI_cmd_list.CopyToResolveTarget(
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
    );*/

    // DEBUG READ-BACK
     {
       FRHIResourceCreateInfo CreateInfo;
       FTexture2DRHIRef readback_tex = RHICreateTexture2D(
           NN,
           NN,
         PF_FloatRGBA,
         1,
         1,
         TexCreate_RenderTargetable,
         CreateInfo);

       RHI_cmd_list.CopyToResolveTarget(
           eWave_h->GetRenderTargetResource()->TextureRHI,
         readback_tex->GetTexture2D(),
         FResolveParams()
       );

       UE_LOG(LogTemp, Warning, TEXT("READBACK START"));

       FReadSurfaceDataFlags read_flags(RCM_MinMax);
       read_flags.SetLinearToGamma(false);

       TArray<FFloat16Color> rdata;
       RHI_cmd_list.ReadSurfaceFloatData(
         readback_tex->GetTexture2D(),
         FIntRect(0, 0, NN, NN),
         rdata,
         read_flags
       );

       UE_LOG(LogTemp, Warning, TEXT("Amount of pixels: %i"), rdata.Num());
       for (int i = 0; i < rdata.Num(); i++) {
         UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, rdata[i].R.GetFloat(), rdata[i].G.GetFloat(), rdata[i].B.GetFloat(), rdata[i].A.GetFloat());
       }
       UE_LOG(LogTemp, Warning, TEXT("READBACK END"));
     }

}