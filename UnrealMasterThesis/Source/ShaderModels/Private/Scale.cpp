#include "Scale.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256
#define TEMP_TEXTURE_N 256

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
    UTextureRenderTarget2D* input_output_rtt,
    UTextureRenderTarget2D* copy_rtt,
    float scale) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<ScaleShader::FParameters>();

	FRDGTextureRef io_tex_ref = register_texture4(graph_builder, input_output_rtt, "InputOutputRenderTarget");
    auto uav = graph_builder.CreateUAV(io_tex_ref);
    FRDGTextureRef io_tex_ref2 = register_texture4(graph_builder, copy_rtt, "CopyRenderTarget");
    auto uav2 = graph_builder.CreateUAV(io_tex_ref2);

    PassParameters->input_output_rtt = uav;
    PassParameters->copy_rtt = uav2;
    PassParameters->scale = scale;
    

    TShaderMapRef<ScaleShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("Scale Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(TEMP_TEXTURE_N, TEMP_TEXTURE_N, 1)
    );

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget_Scale;
    graph_builder.QueueTextureExtraction(io_tex_ref, &PooledComputeTarget_Scale);
    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget_Scale2;
    graph_builder.QueueTextureExtraction(io_tex_ref2, &PooledComputeTarget_Scale2);

    graph_builder.Execute();

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget_Scale.GetReference()->GetRenderTargetItem().TargetableTexture,
        input_output_rtt->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );
    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget_Scale2.GetReference()->GetRenderTargetItem().TargetableTexture,
        copy_rtt->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );
   
    /*UE_LOG(LogTemp, Warning, TEXT("SCALE OUTPUT START"));
    ReadbackRTT3(RHI_cmd_list, input_output_rtt);
    UE_LOG(LogTemp, Warning, TEXT("FFT SCALE process OUTPUT END"));*/
}