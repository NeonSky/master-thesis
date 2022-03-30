#include "ShaderModels.h"

#include "Globals/StatelessHelpers.h"

#include "Butterfly.h"
#include "ButterflyPostProcess.h"
#include "ButterflyPostProcessForward.h"
#include "eWave.h"
#include "Clear.h"
#include "Scale.h"
#include "Serialize.h"
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

		shader2->BuildAndExecuteGraph(
			RHI_cmd_list,
			output_param
		);

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

void ShaderModelsModule::Buildh0Textures(int N, float L, std::function<float (FVector2D)> wave_spectrum, int seed) {
 	TShaderMapRef<FourierComponentsShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, N, L, wave_spectrum, seed](FRHICommandListImmediate& RHI_cmd_list) {
			shader->Buildh0Textures(N, L, wave_spectrum, seed);
		}); 

}

void ShaderModelsModule::ComputeFourierComponents(
	float t,
	UTextureRenderTarget2D* tilde_hkt_dy,
	UTextureRenderTarget2D* tilde_hkt_dxz) {

 	TShaderMapRef<FourierComponentsShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* tilde_hkt_dy_param = tilde_hkt_dy;
	UTextureRenderTarget2D* tilde_hkt_dxz_param = tilde_hkt_dxz;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, t, tilde_hkt_dy_param, tilde_hkt_dxz_param](FRHICommandListImmediate& RHI_cmd_list) {
			shader->BuildAndExecuteGraph(
				RHI_cmd_list,
				t,
				tilde_hkt_dy_param,
				tilde_hkt_dxz_param
			);
		}); 
}

void ShaderModelsModule::ComputeeWave(
	float dt, 
	UTextureRenderTarget2D* eWave_hv) {

	TShaderMapRef<eWaveShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* eWave_hv_param = eWave_hv;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, dt, eWave_hv_param](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			dt,
			eWave_hv_param
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
	UTextureRenderTarget2D* copy_rtt_param = copy_rtt;
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

void ShaderModelsModule::ComputeSerialization(UTextureRenderTarget2D* input_rtt, UTextureRenderTarget2D* serialize_rtt, TArray<float>& out_param) {
	TShaderMapRef<SerializeShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* input_rtt_param = input_rtt;
	UTextureRenderTarget2D* serialize_rtt_param = serialize_rtt;

	TArray<FFloat16Color> data;
	TArray<FFloat16Color>* data_ptr = &data;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, input_rtt_param, serialize_rtt_param, data_ptr](FRHICommandListImmediate& RHI_cmd_list) {
		shader->BuildAndExecuteGraph(
			RHI_cmd_list,
			input_rtt_param,
			serialize_rtt_param,
			data_ptr
		);
	});

	FRenderCommandFence fence;
	fence.BeginFence();
	fence.Wait();

	for (auto color16 : data) {
		float color = RECOVER_F32(color16);
		out_param.Add(color);
	}
}

void ShaderModelsModule::ComputeObstruction(
    UTextureRenderTarget2D* boat_rtt,
	TRefCountPtr<FRDGPooledBuffer> submerged_triangles,
	UTextureRenderTarget2D* obstructionMap_rtt,
	UTextureRenderTarget2D* hv_rtt,
	UTextureRenderTarget2D* hv_prev_rtt,
	int preFFT) {

	TShaderMapRef<ObstructionShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* obstructionMap_rtt_param = obstructionMap_rtt;
	UTextureRenderTarget2D* hv_rtt_param = hv_rtt;
	UTextureRenderTarget2D* hv_prev_rtt_param = hv_prev_rtt;

	ENQUEUE_RENDER_COMMAND(shader)(
		[shader, boat_rtt, submerged_triangles, obstructionMap_rtt_param, hv_rtt_param, hv_prev_rtt_param, preFFT](FRHICommandListImmediate& RHI_cmd_list) {
		// UE_LOG(LogTemp, Warning, TEXT("is valid?: %i"), submerged_triangles.IsValid()); // mostly 1 but sometimes 0
			if (!submerged_triangles.IsValid()) {
				UE_LOG(LogTemp, Warning, TEXT("Not valid3"));
				return;
			}
			shader->BuildAndExecuteGraph(
				RHI_cmd_list,
				boat_rtt,
				submerged_triangles,
				obstructionMap_rtt_param,
				hv_rtt_param,
				hv_prev_rtt_param,
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
	TArray<UTextureRenderTarget2D*> wake_textures,
	UTextureRenderTarget2D* boat_texture,
	TArray<UTextureRenderTarget2D*> other_boat_textures,
	UTextureRenderTarget2D* readback_texture,
	AActor* update_target,
	std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) {

	TArray<FFloat16Color> data;
	{
		TShaderMapRef<SubmergedTrianglesShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		ENQUEUE_RENDER_COMMAND(shader)(
			[callback, shader, speed_input, velocity_input, collision_mesh, elevation_texture, wake_textures, boat_texture, other_boat_textures, readback_texture, update_target, &data](FRHICommandListImmediate& RHI_cmd_list) {

				TRefCountPtr<FRDGPooledBuffer> submerged_triangles_buffer;
				shader->BuildAndExecuteGraph(
					RHI_cmd_list,
					collision_mesh,
					elevation_texture,
					boat_texture,
					other_boat_textures,
					wake_textures,
					&submerged_triangles_buffer
				);

				if (!submerged_triangles_buffer.IsValid()) {
					UE_LOG(LogTemp, Error, TEXT("Submerged triangles buffer is not valid. This should never happen."));
					return;
				}

				TShaderMapRef<GPUBoatShader> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				ENQUEUE_RENDER_COMMAND(shader2)(
					[callback, shader2, speed_input, velocity_input, elevation_texture, submerged_triangles_buffer, boat_texture, readback_texture, update_target, &data](FRHICommandListImmediate& RHI_cmd_list) { // works

					shader2->BuildAndExecuteGraph(
						RHI_cmd_list,
						speed_input,
						velocity_input,
						elevation_texture,
						submerged_triangles_buffer,
						boat_texture,
						readback_texture,
						update_target ? (&data) : nullptr
					);

					callback(submerged_triangles_buffer);

				});

			}); 
	}

	// Optional. Only used for the current camera workaround (see report).
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