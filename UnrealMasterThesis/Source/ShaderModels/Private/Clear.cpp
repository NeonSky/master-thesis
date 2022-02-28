#include "Clear.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256

IMPLEMENT_GLOBAL_SHADER(ClearShader, "/Project/UnrealMasterThesis/Clear.usf", "eWaveCompute", SF_Compute);

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

void ClearShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    UTextureRenderTarget2D* result) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<ClearShader::FParameters>();

    FRDGTextureRef io_tex_ref = register_texture32(graph_builder, result, "result");
    PassParameters->result = graph_builder.CreateUAV(io_tex_ref);

    TShaderMapRef<ClearShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("Add Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(NN, NN, 1)
    );

    graph_builder.Execute();
}