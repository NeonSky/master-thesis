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

void eWaveShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    float dt,
    UTextureRenderTarget2D* eWave_hv,
    UTextureRenderTarget2D* eWave_hv_copy) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<eWaveShader::FParameters>();

    PassParameters->dt = dt;

    FRDGTextureRef io_tex_ref_hv = register_texture5(graph_builder, eWave_hv, "eWave height and velocity potential");
    auto uav_hv = graph_builder.CreateUAV(io_tex_ref_hv);

    FRDGTextureRef copy_tex_ref_hv = register_texture5(graph_builder, eWave_hv_copy, "eWave height and velocity potential COPY");
    auto uav_hv_copy = graph_builder.CreateUAV(copy_tex_ref_hv);

    PassParameters->eWave_hv = uav_hv;
    PassParameters->eWave_hv_copy = uav_hv_copy;

    TShaderMapRef<eWaveShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("eWave Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(NN, NN, 1)
    );

    graph_builder.Execute();
}
