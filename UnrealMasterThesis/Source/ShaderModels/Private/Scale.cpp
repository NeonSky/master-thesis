#include "Scale.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256
#define TEMP_TEXTURE_N 4

IMPLEMENT_GLOBAL_SHADER(ScaleShader, "/Project/UnrealMasterThesis/Scale.usf", "eWaveCompute", SF_Compute);

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

struct CustomUAV {
    FRDGTextureRef ref;
    FRDGTextureUAVRef uav_ref;
};

CustomUAV create_UAV4( // TODO: this and register texture defined in fourier components.
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

void ScaleShader::BuildTestTextures(int N, float L) {
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
                    pixel_data.Add(FFloat16Color(FLinearColor(1.0, 0.0, 0.0, 1.0)));
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

        //this->test = Texture2DRHI;
    }
}

void ReadbackRTT3(FRHICommandListImmediate& RHI_cmd_list, UTextureRenderTarget2D* rtt) {
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

void ScaleShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    UTextureRenderTarget2D* input_rtt,
    UTextureRenderTarget2D* output_rtt,
    float scale_real,
    float scale_imag) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<ScaleShader::FParameters>();

	FRDGTextureRef io_tex_ref = register_texture4(graph_builder, input_rtt, "InputOutputRenderTarget");
    auto uav = graph_builder.CreateUAV(io_tex_ref);

    PassParameters->input_rtt = uav;
    PassParameters->output_rtt = uav;
    PassParameters->scale_real = scale_real;
    PassParameters->scale_imag = scale_imag;
    

    TShaderMapRef<ScaleShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("Scale Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(TEMP_TEXTURE_N, TEMP_TEXTURE_N, 1)
    );

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget2_Scale;
    graph_builder.QueueTextureExtraction(io_tex_ref, &PooledComputeTarget2_Scale);

    graph_builder.Execute();

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget2_Scale.GetReference()->GetRenderTargetItem().TargetableTexture,
        input_rtt->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );
   
    UE_LOG(LogTemp, Warning, TEXT("SCALE OUTPUT START"));
    ReadbackRTT3(RHI_cmd_list, input_rtt);
    UE_LOG(LogTemp, Warning, TEXT("FFT SCALE process OUTPUT END"));
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
         output_rtt->GetRenderTargetResource()->TextureRHI,
         readback_tex->GetTexture2D(),
         FResolveParams()
       );

       UE_LOG(LogTemp, Warning, TEXT("READBACK START - Scale.cpp, after scale pass"));

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
     }*/

}