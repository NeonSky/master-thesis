#include "ShaderModels.h"

#include "Butterfly.h"
#include "ButterflyTexture.h"
#include "FourierComponents.h"
#include "ButterflyPostProcess.h"
#include "ButterflyPostProcessForward.h"
#include "eWave.h"
#include "Add.h"
#include "Scale.h"
#include "Obstruction.h"
#include "ElevationSampler.h"
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


void ShaderModelsModule::FFT_Forward(UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* output) {
	TShaderMapRef<ButterflyShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<ButterflyPostProcessShaderForward> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* butterfly_param = butterfly;
	UTextureRenderTarget2D* output_param = output;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, butterfly_param, output_param, shader2](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			butterfly_param,
			output_param
		);

		shader2->BuildAndExecuteGraph(
			RHI_cmd_list,
			output_param
		);
	});
}

void ShaderModelsModule::Buildh0Textures(int N, float L, std::function<float (FVector2D)> wave_spectrum) {
 	TShaderMapRef<FourierComponentsShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, N, L, wave_spectrum](FRHICommandListImmediate& RHI_cmd_list) {
			shader->Buildh0Textures(N, L, wave_spectrum);
		}); 

	TShaderMapRef<AddShader> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	ENQUEUE_RENDER_COMMAND(shader2)(
		[shader2, N, L](FRHICommandListImmediate& RHI_cmd_list) {
		shader2->BuildTestTextures(N, L);
	});


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
	UTextureRenderTarget2D* eWave_v) {

	TShaderMapRef<eWaveShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* eWave_h_param = eWave_h;
	UTextureRenderTarget2D* eWave_v_param = eWave_v;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, t, L, eWave_h_param, eWave_v_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			t,
			L,
			eWave_h_param,
			eWave_v_param
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
	UTextureRenderTarget2D* input_output_rtt, 
	UTextureRenderTarget2D* copy_rtt,
	float scale) {

	TShaderMapRef<ScaleShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* input_output_rtt_param = input_output_rtt;
	UTextureRenderTarget2D* copy_rtt_param = input_output_rtt;
	float scale_param = scale;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, input_output_rtt_param, copy_rtt_param, scale_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			input_output_rtt_param,
			copy_rtt_param,
			scale_param
		);
	});

}

void ShaderModelsModule::ComputeObstruction(
	TArray<FVector4> SubmergedTriangles,
	int L,
	UTextureRenderTarget2D* obstructionMap_rtt,
	UTextureRenderTarget2D* h_rtt,
	UTextureRenderTarget2D* v_rtt,
	UTextureRenderTarget2D* hPrev_rtt,
	UTextureRenderTarget2D* vPrev_rtt,
	float xPos,
	float yPos,
	float boat_dx,
	float boat_dy,
	int speedScale,
	int preFFT) {

	TShaderMapRef<ObstructionShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* obstructionMap_rtt_param = obstructionMap_rtt;
	UTextureRenderTarget2D* h_rtt_param = h_rtt;
	UTextureRenderTarget2D* v_rtt_param = v_rtt;
	UTextureRenderTarget2D* hPrev_rtt_param = h_rtt;
	UTextureRenderTarget2D* vPrev_rtt_param = v_rtt;
	float xPos_param = xPos;
	float yPos_param = yPos;
	float boat_dx_param = boat_dx;
	float boat_dy_param = boat_dy;
	int L_param = L;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, SubmergedTriangles, L_param, obstructionMap_rtt_param, h_rtt_param, v_rtt_param, hPrev_rtt_param, vPrev_rtt_param, xPos_param, yPos_param, boat_dx_param, boat_dy_param, speedScale, preFFT](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			SubmergedTriangles,
			SubmergedTriangles.Num() / 3, // the number of triangles is num verts / 3
			L_param,
			obstructionMap_rtt_param,
			h_rtt_param,
			v_rtt_param,
			hPrev_rtt_param,
			vPrev_rtt_param,
			xPos_param,
			yPos_param,
			boat_dx_param,
			boat_dy_param,
			speedScale,
			preFFT
		);
	});
}
	
void ShaderModelsModule::SampleElevationPoints(UTextureRenderTarget2D* elevations, TArray<FVector2D> input_sample_coordinates, TArray<float>* output) {

 	TShaderMapRef<ElevationSamplerShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FRenderCommandFence fence;
	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, elevations, input_sample_coordinates, output](FRHICommandListImmediate& RHI_cmd_list) {
			shader->BuildAndExecuteGraph(
				RHI_cmd_list,
				elevations,
				input_sample_coordinates,
				output
			);
		}); 

	// Force the output to be ready since UE will not allow the render thread to get 2 frames behind the game thread anyway. 
	fence.BeginFence();
	fence.Wait();
}

IMPLEMENT_MODULE(ShaderModelsModule, ShaderModels);