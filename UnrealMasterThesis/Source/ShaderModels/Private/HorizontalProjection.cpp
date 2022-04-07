#include "HorizontalProjection.h"

IMPLEMENT_GLOBAL_SHADER(HorizontalProjectionVertShader, "/Project/UnrealMasterThesis/HorizontalProjection.usf", "MainVertShader", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(HorizontalProjectionFragShader, "/Project/UnrealMasterThesis/HorizontalProjection.usf", "MainFragShader", SF_Pixel);

TGlobalResource<ScreenQuad> GScreenQuad;

void HorizontalProjectionFragShader::RenderTo(
    FRHICommandListImmediate& RHICmdList,
    TRefCountPtr<FRDGPooledBuffer> submerged_position_buffer,
    UTextureRenderTarget2D* output) {

	FRHIRenderPassInfo RenderPassInfo(output->GetRenderTargetResource()->GetRenderTargetTexture(), ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("OutputToRenderTarget"));

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<HorizontalProjectionVertShader> VertShader(ShaderMap);
	TShaderMapRef<HorizontalProjectionFragShader> FragShader(ShaderMap);
		
	// Pipeline state
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = FragShader.GetPixelShader();
	GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

    // Set vertex buffer
    // FStaticMeshLODResources& mesh_res = collision_mesh->GetStaticMeshComponent()->GetStaticMesh()->RenderData->LODResources[0];
	// RHICmdList.SetStreamSource(0, mesh_res.StaticMeshVertexBuffer.FTexcoordVertexBuffer.VertexBufferRHI, 0);

	// RHICmdList.SetStreamSource(0, submerged_triangles.GetVertexBufferRHI(), 0);
    RHICmdList.SetStreamSource(0, GScreenQuad.VertexBufferRHI, 0);

	RHICmdList.DrawPrimitive(0, 2, 1);

	RHICmdList.EndRenderPass();
}