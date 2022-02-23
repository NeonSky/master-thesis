#include "ShaderModels.h"

#include "Butterfly.h"
#include "ButterflyTexture.h"
#include "FourierComponents.h"
#include "ButterflyPostProcess.h"
#include "ButterflyPostProcessForward.h"
#include "eWave.h"
#include "Add.h"
#include "Scale.h"

#include "GlobalShader.h"
#include "ShaderCore.h" 
#include "Engine/TextureRenderTarget2D.h"

void ShaderModelsModule::StartupModule() {
	UE_LOG(LogTemp, Warning, TEXT("ShaderModelsModule::StartupModule()"));

  FString shader_dir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
  AddShaderSourceDirectoryMapping(TEXT("/Project/UnrealMasterThesis"), shader_dir);
}

void ShaderModelsModule::ShutdownModule() {
	UE_LOG(LogTemp, Warning, TEXT("ShaderModelsModule::ShutdownModule()"));
}

void ShaderModelsModule::GenerateButterflyTexture(UTextureRenderTarget2D* output) {

 	TShaderMapRef<ButterflyTextureShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* output_param = output;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, output_param](FRHICommandListImmediate& RHI_cmd_list) {
			shader->BuildAndExecuteGraph(
				RHI_cmd_list,
				output_param
			);
		}); 
}

void ShaderModelsModule::FFT(UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* output, float scale) {
 	TShaderMapRef<ButterflyShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	TShaderMapRef<ButterflyPostProcessShader> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* butterfly_param = butterfly;
	UTextureRenderTarget2D* output_param    = output;
	float scale_param = scale;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, butterfly_param, output_param, shader2, scale_param](FRHICommandListImmediate& RHI_cmd_list) {
			shader->BuildAndExecuteGraph(
				RHI_cmd_list,
				butterfly_param,
				output_param
			);

			shader2->BuildAndExecuteGraph(
				RHI_cmd_list,
				output_param,
				-1,
				1
			);
		});
}


void ShaderModelsModule::FFT_Forward(UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* output, float scale) {
	TShaderMapRef<ButterflyShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<ButterflyPostProcessShaderForward> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* butterfly_param = butterfly;
	UTextureRenderTarget2D* output_param = output;
	float scale_param = scale;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, butterfly_param, output_param, shader2, scale_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			butterfly_param,
			output_param
		);

		shader2->BuildAndExecuteGraph(
			RHI_cmd_list,
			output_param,
			-1,
			1
		);
	});
}

void ShaderModelsModule::FFT2(UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* output, float scale_real) {
	TShaderMapRef<ButterflyShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<ButterflyPostProcessShader> shader2_post_process(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<ScaleShader> shader3_scale(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* butterfly_param = butterfly;
	UTextureRenderTarget2D* output_param = output;
	float scale_real_param = scale_real;
	float scale_imag_param = -1.0;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, butterfly_param, output_param, shader2_post_process, shader3_scale, scale_real_param, scale_imag_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader3_scale->BuildAndExecuteGraph(
			RHI_cmd_list,
			output_param,
			output_param, // write to the same render target
			1.0,
			-1.0
		);

		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			butterfly_param,
			output_param
		);

		shader2_post_process->BuildAndExecuteGraph(
			RHI_cmd_list,
			output_param,
			42,
			-42

		);

		shader3_scale->BuildAndExecuteGraph(
			RHI_cmd_list,
			output_param,
			output_param, // write to the same render target
			scale_real_param,
			-1.0 * scale_real_param
		);
	});
}

void ShaderModelsModule::Buildh0Textures(int N, float L, std::function<float (FVector2D)> wave_spectrum) {
 	TShaderMapRef<FourierComponentsShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, N, L, wave_spectrum](FRHICommandListImmediate& RHI_cmd_list) {
			shader->Buildh0Textures(N, L, wave_spectrum);
		}); 

	//TShaderMapRef<AddShader> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	/*ENQUEUE_RENDER_COMMAND(shader2)(
		[shader2, N, L](FRHICommandListImmediate& RHI_cmd_list) {
		shader2->BuildTestTextures(N, L);
	});*/


}

void ShaderModelsModule::ComputeFourierComponents(
	float t,
	float L,
	UTextureRenderTarget2D* tilde_hkt_dx,
	UTextureRenderTarget2D* tilde_hkt_dy,
	UTextureRenderTarget2D* tilde_hkt_dz) {

 	TShaderMapRef<FourierComponentsShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* tilde_hkt_dx_param = tilde_hkt_dx;
	UTextureRenderTarget2D* tilde_hkt_dy_param = tilde_hkt_dy;
	UTextureRenderTarget2D* tilde_hkt_dz_param = tilde_hkt_dz;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, t, L, tilde_hkt_dx_param, tilde_hkt_dy_param, tilde_hkt_dz_param](FRHICommandListImmediate& RHI_cmd_list) {
			shader->BuildAndExecuteGraph(
				RHI_cmd_list,
				t,
				L,
				tilde_hkt_dx_param,
				tilde_hkt_dy_param,
				tilde_hkt_dz_param
			);
		}); 
}

void ShaderModelsModule::ComputeeWave(
	float t, 
	float L, 
	UTextureRenderTarget2D* eWave_h, 
	UTextureRenderTarget2D* eWave_hPrev,
	UTextureRenderTarget2D* eWave_v, 
	UTextureRenderTarget2D* eWave_vPrev) {

	TShaderMapRef<eWaveShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* eWave_h_param = eWave_h;
	UTextureRenderTarget2D* eWave_hPrev_param = eWave_hPrev;
	UTextureRenderTarget2D* eWave_v_param = eWave_v;
	UTextureRenderTarget2D* eWave_vPrev_dz_param = eWave_vPrev;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, t, L, eWave_h_param, eWave_hPrev_param, eWave_v_param, eWave_vPrev_dz_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			t,
			L,
			eWave_h_param,
			eWave_hPrev_param,
			eWave_v_param,
			eWave_vPrev_dz_param
		);
	});

}

void ShaderModelsModule::ComputeAdd(
	UTextureRenderTarget2D* term1,
	UTexture2D* term2,
	UTextureRenderTarget2D* result) {

	TShaderMapRef<AddShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* term1_param = term1;
	UTexture2D* term2_param = term2;
	UTextureRenderTarget2D* result_param = result;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, term1_param, term2_param, result_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			term1_param,
			term2_param,
			result_param
		);
	});

}

void ShaderModelsModule::ComputeScale(
	UTextureRenderTarget2D* input_rtt,
	UTextureRenderTarget2D* output_rtt,
	float scale_real,
	float scale_imag) {

	TShaderMapRef<ScaleShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* input_rtt_param = input_rtt;
	UTextureRenderTarget2D* output_rtt_param = output_rtt;
	float scale_real_param = scale_real;
	float scale_imag_param = scale_imag;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, input_rtt_param, output_rtt_param, scale_real_param, scale_imag_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			input_rtt_param,
			output_rtt_param,
			scale_real_param,
			scale_imag_param
		);
	});

}

IMPLEMENT_MODULE(ShaderModelsModule, ShaderModels);