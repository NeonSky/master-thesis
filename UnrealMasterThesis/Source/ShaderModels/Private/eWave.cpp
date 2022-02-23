#include "eWave.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256
#define TEMP_TEXTURE_N 256

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
    UTextureRenderTarget2D* eWave_v) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<eWaveShader::FParameters>();

    PassParameters->N = NN; // TODO
    PassParameters->L = L;
    PassParameters->t = t;

    FRDGTextureRef io_tex_ref_h = register_texture5(graph_builder, eWave_h, "eWave Height");
    FRDGTextureRef io_tex_ref_v = register_texture5(graph_builder, eWave_v, "eWave Velocity Potential");
    auto uav_h = graph_builder.CreateUAV(io_tex_ref_h);
    auto uav_v = graph_builder.CreateUAV(io_tex_ref_v);

    PassParameters->eWave_h = uav_h;
    PassParameters->eWave_v = uav_v;

    TShaderMapRef<eWaveShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("eWave Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(NN, NN, 1)
    );

    /*TRefCountPtr<IPooledRenderTarget> PooledComputeTarget_ewave_h;
    graph_builder.QueueTextureExtraction(io_tex_ref_h, &PooledComputeTarget_ewave_h);
    
    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget_ewave_v;
    graph_builder.QueueTextureExtraction(io_tex_ref_v, &PooledComputeTarget_ewave_v);*/

    graph_builder.Execute();

    /*RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget_ewave_h.GetReference()->GetRenderTargetItem().TargetableTexture,
        eWave_h->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget_ewave_v.GetReference()->GetRenderTargetItem().TargetableTexture,
        eWave_v->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );*/

    // DEBUG READ-BACK
     //{
     //  FRHIResourceCreateInfo CreateInfo;
     //  FTexture2DRHIRef readback_tex = RHICreateTexture2D(
     //      NN,
     //      NN,
     //    PF_FloatRGBA,
     //    1,
     //    1,
     //    TexCreate_RenderTargetable,
     //    CreateInfo);

     //  RHI_cmd_list.CopyToResolveTarget(
     //      eWave_h->GetRenderTargetResource()->TextureRHI,
     //    readback_tex->GetTexture2D(),
     //    FResolveParams()
     //  );

     //  //UE_LOG(LogTemp, Warning, TEXT("READBACK START"));

     //  FReadSurfaceDataFlags read_flags(RCM_MinMax);
     //  read_flags.SetLinearToGamma(false);

     //  TArray<FFloat16Color> rdata;
     //  RHI_cmd_list.ReadSurfaceFloatData(
     //    readback_tex->GetTexture2D(),
     //    FIntRect(0, 0, NN, NN),
     //    rdata,
     //    read_flags
     //  );

     // 
     //}
     /*UE_LOG(LogTemp, Warning, TEXT("Amount of pixels: %i"), rdata.Num());
      for (int i = 0; i < rdata.Num(); i++) {
        UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, rdata[i].R.GetFloat(), rdata[i].G.GetFloat(), rdata[i].B.GetFloat(), rdata[i].A.GetFloat());
      }
      UE_LOG(LogTemp, Warning, TEXT("READBACK END"));*/

}