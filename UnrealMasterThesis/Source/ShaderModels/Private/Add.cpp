#include "Add.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 128
#define TEMP_TEXTURE_N 128

IMPLEMENT_GLOBAL_SHADER(AddShader, "/Project/UnrealMasterThesis/Add.usf", "eWaveCompute", SF_Compute);

FRDGTextureRef register_texture32(
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

void AddShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    UTextureRenderTarget2D* term1,
    UTexture2D* term2,
    UTextureRenderTarget2D* result) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<AddShader::FParameters>();

    auto test2 = register_texture32(graph_builder, result, "result");
    PassParameters->result = graph_builder.CreateUAV(test2);

    TShaderMapRef<AddShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("Add Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(TEMP_TEXTURE_N, TEMP_TEXTURE_N, 1)
    );
    
   /* TRefCountPtr<IPooledRenderTarget> PooledComputeTarget3_Add;
    graph_builder.QueueTextureExtraction(test2, &PooledComputeTarget3_Add);*/

    graph_builder.Execute();

    /*RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget3_Add.GetReference()->GetRenderTargetItem().TargetableTexture,
        result->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );*/

     //DEBUG READ-BACK
     /*{
       FRHIResourceCreateInfo CreateInfo;
       FTexture2DRHIRef readback_tex = RHICreateTexture2D(
         TEMP_TEXTURE_N,
         TEMP_TEXTURE_N,
         PF_FloatRGBA,
         1,
         1,
         TexCreate_RenderTargetable,
         CreateInfo);

       RHI_cmd_list.CopyToResolveTarget(
         result->GetRenderTargetResource()->TextureRHI,
         readback_tex->GetTexture2D(),
         FResolveParams()
       );

       UE_LOG(LogTemp, Warning, TEXT("READBACK START - Add.cpp, after add pass"));

       FReadSurfaceDataFlags read_flags(RCM_MinMax);
       read_flags.SetLinearToGamma(false);

       TArray<FFloat16Color> rdata;
       RHI_cmd_list.ReadSurfaceFloatData(
         readback_tex->GetTexture2D(),
         FIntRect(0, 0, TEMP_TEXTURE_N, TEMP_TEXTURE_N),
         rdata,
         read_flags
       );

       UE_LOG(LogTemp, Warning, TEXT("Amount of pixels: %i"), rdata.Num());
       for (int i = 0; i < 32; i++) {
         UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, rdata[i].R.GetFloat(), rdata[i].G.GetFloat(), rdata[i].B.GetFloat(), rdata[i].A.GetFloat());
       }
       UE_LOG(LogTemp, Warning, TEXT("READBACK END"));
     }*/

}