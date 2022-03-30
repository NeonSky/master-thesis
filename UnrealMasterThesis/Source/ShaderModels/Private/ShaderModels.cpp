#include "ShaderModels.h"

#include "Globals/StatelessHelpers.h"

#include "Butterfly.h"
#include "ButterflyPostProcess.h"
#include "ButterflyPostProcessForward.h"
#include "eWave.h"
#include "Clear.h"
#include "Scale.h"
#include "Obstruction.h"
#include "ElevationSampler.h"
#include "ButterflyTexture.h"
#include "FourierComponents.h"
#include "GPUBoat.h"
#include "SubmergedTriangles.h"

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

void ShaderModelsModule::Clear(UTextureRenderTarget2D* result) {

	TShaderMapRef<ClearShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* result_param = result;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, result_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
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
    UTextureRenderTarget2D* boat_rtt,
	TRefCountPtr<FRDGPooledBuffer> submerged_triangles,
	UTextureRenderTarget2D* obstructionMap_rtt,
	UTextureRenderTarget2D* h_rtt,
	UTextureRenderTarget2D* v_rtt,
	UTextureRenderTarget2D* hPrev_rtt,
	UTextureRenderTarget2D* vPrev_rtt,
	int preFFT) {

	TShaderMapRef<ObstructionShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* obstructionMap_rtt_param = obstructionMap_rtt;
	UTextureRenderTarget2D* h_rtt_param = h_rtt;
	UTextureRenderTarget2D* v_rtt_param = v_rtt;
	UTextureRenderTarget2D* hPrev_rtt_param = h_rtt;
	UTextureRenderTarget2D* vPrev_rtt_param = v_rtt;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, boat_rtt, submerged_triangles, obstructionMap_rtt_param, h_rtt_param, v_rtt_param, hPrev_rtt_param, vPrev_rtt_param, preFFT](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			boat_rtt,
			submerged_triangles,
			obstructionMap_rtt_param,
			h_rtt_param,
			v_rtt_param,
			hPrev_rtt_param,
			vPrev_rtt_param,
			preFFT
		);
	});
}
	
void ShaderModelsModule::SampleElevationPoints(
	UTextureRenderTarget2D* elevations,
	TArray<UTextureRenderTarget2D*> wake_rtts,
	TArray<FVector2D> ws_boat_coords,
	TArray<FVector2D> input_sample_coordinates,
	TArray<float>* output) {

 	TShaderMapRef<ElevationSamplerShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FRenderCommandFence fence;
	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, elevations, wake_rtts, ws_boat_coords, input_sample_coordinates, output](FRHICommandListImmediate& RHI_cmd_list) {
			shader->BuildAndExecuteGraph(
				RHI_cmd_list,
				elevations,
				wake_rtts,
				ws_boat_coords,
				input_sample_coordinates,
				output
			);
		}); 

	// Force the output to be ready since UE will not allow the render thread to get 2 frames behind the game thread anyway. 
	fence.BeginFence();
	fence.Wait();
}

void ShaderModelsModule::ResetGPUBoat(UTextureRenderTarget2D* input_output) {

 	TShaderMapRef<GPUBoatShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	ENQUEUE_RENDER_COMMAND(shader)([shader, input_output](FRHICommandListImmediate& RHI_cmd_list) {
		shader->ResetBoatTexture(RHI_cmd_list, input_output);
	});

	FRenderCommandFence fence;
	fence.BeginFence();
	fence.Wait();

}

void ShaderModelsModule::UpdateGPUBoat(
    float speed_input,
    FVector2D velocity_input,
	AStaticMeshActor* collision_mesh,
	UTextureRenderTarget2D* elevation_texture,
	UTextureRenderTarget2D* wake_texture,
	UTextureRenderTarget2D* input_output,
	UTextureRenderTarget2D* readback_texture,
	TRefCountPtr<FRDGPooledBuffer>& submerged_triangles_buffer,
	AActor* update_target) {

	// bool flag = false;
	// TRefCountPtr<FGraphEvent> event;

	submerged_triangles_buffer = nullptr;

	// auto callback = [&](TRefCountPtr<FRDGPooledBuffer> submerged_triangles){
	// 	UE_LOG(LogTemp, Warning, TEXT("This is a test"));
	// 	UE_LOG(LogTemp, Warning, TEXT("valid? %i"), submerged_triangles.IsValid());

	// 	TArray<FFloat16Color> data;
	// 	TShaderMapRef<GPUBoatShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	// 	ENQUEUE_RENDER_COMMAND(shader)(
	// 		[shader, speed_input, velocity_input, elevation_texture, submerged_triangles, input_output, readback_texture, update_target, &data](FRHICommandListImmediate& RHI_cmd_list) { // works
	// 		// [shader, speed_input, submerged_triangles](FRHICommandListImmediate& RHI_cmd_list) { // works
	// 		// [shader, submerged_triangles](FRHICommandListImmediate& RHI_cmd_list) { // works

	// 		UE_LOG(LogTemp, Warning, TEXT("valid2? %i"), submerged_triangles.IsValid()); // 1
	// 	// 		// // UE_LOG(LogTemp, Warning, TEXT("step 4: %i"), submerged_triangles_buffer.IsValid()); // 1
	// 	// 		// double start = FPlatformTime::Seconds();
	// 	// 		// // while (!flag) {}
	// 	// 		// while (!submerged_triangles_buffer.IsValid()) {}
	// 	// 		// // while (!submerged_triangles_buffer.IsValid()) {}
	// 	// 		// double end = FPlatformTime::Seconds();
	// 	// 		// UE_LOG(LogTemp, Warning, TEXT("code executed in %f seconds."), end-start);

	// 			shader->BuildAndExecuteGraph(
	// 				RHI_cmd_list,
	// 				speed_input,
	// 				velocity_input,
	// 				elevation_texture,
	// 				submerged_triangles,
	// 				input_output,
	// 				readback_texture,
	// 				update_target ? (&data) : nullptr
	// 			);
	// 		}); 
	// };

	// UE_LOG(LogTemp, Warning, TEXT("step 1: %i"), submerged_triangles_buffer.IsValid()); // 0
	TArray<FFloat16Color> data;
	{
		TShaderMapRef<SubmergedTrianglesShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		// TShaderMapRef<GPUBoatShader> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		// TODO: mutex?

		ENQUEUE_RENDER_COMMAND(shader)(
			// [callback, shader, shader2, speed_input, velocity_input, collision_mesh, elevation_texture, wake_texture, input_output, &submerged_triangles_buffer, readback_texture, update_target, &data](FRHICommandListImmediate& RHI_cmd_list) {
			[shader, speed_input, velocity_input, collision_mesh, elevation_texture, wake_texture, input_output, &submerged_triangles_buffer, readback_texture, update_target, &data](FRHICommandListImmediate& RHI_cmd_list) {

				// UE_LOG(LogTemp, Warning, TEXT("step 2: %i"), submerged_triangles_buffer.IsValid()); // 0

				shader->BuildAndExecuteGraph(
					RHI_cmd_list,
					collision_mesh,
					elevation_texture,
					input_output,
					wake_texture,
					&submerged_triangles_buffer
				);

				// callback(submerged_triangles_buffer);

				// UE_LOG(LogTemp, Warning, TEXT("step 3: %i"), submerged_triangles_buffer.IsValid()); // 1
				// while (!submerged_triangles_buffer.IsValid()) {}
				// flag = true;

				TShaderMapRef<GPUBoatShader> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				ENQUEUE_RENDER_COMMAND(shader2)(
					[shader2, speed_input, velocity_input, elevation_texture, submerged_triangles_buffer, input_output, readback_texture, update_target, &data](FRHICommandListImmediate& RHI_cmd_list) { // works
					shader2->BuildAndExecuteGraph(
						RHI_cmd_list,
						speed_input,
						velocity_input,
						elevation_texture,
						submerged_triangles_buffer,
						input_output,
						readback_texture,
						update_target ? (&data) : nullptr
					);
				});

				// event = RHI_cmd_list.RenderThreadTaskFence(); // RHIThreadFence

				// RHI_cmd_list.BlockUntilGPUIdle();
				// RHI_cmd_list.FlushResources();
				// RHI_cmd_list.SubmitCommandsAndFlushGPU();
				// RHI_cmd_list.WaitOnRenderThreadTaskFence
			}); 

		// TODO: Remove. This implicitly causes a CPU stall, while we simply want to move a buffer from one shader to another.
		// FRenderCommandFence fence;
		// fence.BeginFence();
		// fence.Wait();
	}

	// while (!flag) {
	// 	UE_LOG(LogTemp, Warning, TEXT("Not yet"));
	// }
	// UE_LOG(LogTemp, Warning, TEXT("Done!"));

	// TArray<FFloat16Color> data;

	if (update_target) {
		FRenderCommandFence fence;
		fence.BeginFence();
		fence.Wait();

		FVector pos = FVector(RECOVER_F32(data[0]), RECOVER_F32(data[1]), RECOVER_F32(data[2]));
		FQuat rot   = FQuat(RECOVER_F32(data[3]), RECOVER_F32(data[4]), RECOVER_F32(data[5]), RECOVER_F32(data[6]));

		// UE_LOG(LogTemp, Warning, TEXT("GPU boat debug: %.9f"), RECOVER_F32(data[7]));

		update_target->SetActorLocation(METERS_TO_UNREAL_UNITS * pos);
		update_target->SetActorRotation(rot, ETeleportType::None);
	}
}

IMPLEMENT_MODULE(ShaderModelsModule, ShaderModels);