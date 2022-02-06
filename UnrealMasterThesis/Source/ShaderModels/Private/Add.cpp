#include "Add.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256
#define TEMP_TEXTURE_N 4

IMPLEMENT_GLOBAL_SHADER(AddShader, "/Project/UnrealMasterThesis/Add.usf", "eWaveCompute", SF_Compute);

FRDGTextureRef register_texture3(
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
}

struct CustomUAV {
    FRDGTextureRef ref;
    FRDGTextureUAVRef uav_ref;
};

CustomUAV create_UAV3( // TODO: this and register texture defined in fourier components.
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

void AddShader::BuildTestTextures(int N, float L) {
    {
        FRHIResourceCreateInfo CreateInfo;
        FTexture2DRHIRef Texture2DRHI = RHICreateTexture2D(
            TEMP_TEXTURE_N,
            TEMP_TEXTURE_N,
            PF_FloatRGBA,
            1,
            1,
            TexCreate_RenderTargetable,
            CreateInfo);

        TArray<FFloat16Color> pixel_data;
        bool flip = true;
        for (int i = 0; i < TEMP_TEXTURE_N; i++) {
            for (int j = 0; j < TEMP_TEXTURE_N; j++) {
                if (flip) {
                    pixel_data.Add(FFloat16Color(FLinearColor(0.1234 * j + 0.4321 * i, 0.0, 0.0, 1.0))); // 0.2 * j + 0.1,    1,    0.1234 * j + 0.4321 * i  ,    i + 1 + j, 0.0, 0.0, 1.0
                }
                else {
                    pixel_data.Add(FFloat16Color(FLinearColor(0.0, 0.0, 0.0, 1.0)));
                }
            }
            flip = !flip;
        }

        uint32 DestStride = 0;
        FFloat16Color* data = (FFloat16Color*)RHILockTexture2D(Texture2DRHI, 0, RLM_WriteOnly, DestStride, false);
        FMemory::Memcpy(data, pixel_data.GetData(), sizeof(FFloat16Color) * pixel_data.Num());
        RHIUnlockTexture2D(Texture2DRHI, 0, false);

        this->test = Texture2DRHI;
    }
}

void AddShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    UTextureRenderTarget2D* term1,
    UTexture2D* term2,
    UTextureRenderTarget2D* result) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<AddShader::FParameters>();


    CustomUAV uavAdd1 = create_UAV3(graph_builder, term1, TEXT("term 1"));
    //CustomUAV uavAdd2 = create_UAV3(graph_builder, term2, TEXT("term 2"));
    CustomUAV uavAdd3 = create_UAV3(graph_builder, result, TEXT("result"));

    PassParameters->term1 = uavAdd1.uav_ref;
    PassParameters->term2 = register_texture3(graph_builder, this->test->GetTexture2D(), "term2"); // uavAdd2.uav_ref;
    // PassParameters->term2 = register_texture3(graph_builder, term2->Resource->GetTexture2DRHI()->GetTexture2D(), "term2");

    PassParameters->result = uavAdd3.uav_ref;

    TShaderMapRef<AddShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("Add Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(TEMP_TEXTURE_N, TEMP_TEXTURE_N, 1)
    );
    
    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget3_Add;
    graph_builder.QueueTextureExtraction(uavAdd3.ref, &PooledComputeTarget3_Add);

    graph_builder.Execute();

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget3_Add.GetReference()->GetRenderTargetItem().TargetableTexture,
        result->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );

     //DEBUG READ-BACK
     {
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
       for (int i = 0; i < rdata.Num(); i++) {
         UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, rdata[i].R.GetFloat(), rdata[i].G.GetFloat(), rdata[i].B.GetFloat(), rdata[i].A.GetFloat());
       }
       UE_LOG(LogTemp, Warning, TEXT("READBACK END"));
     }

}