#include "ShaderModels.h"

#include "Globals/StatelessHelpers.h"

#include "Butterfly.h"
#include "ButterflyPostProcess.h"
#include "ButterflyTexture.h"
#include "ElevationSampler.h"
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

void ShaderModelsModule::FFT(UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* output) {
 	TShaderMapRef<ButterflyShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	TShaderMapRef<ButterflyPostProcessShader> shader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	UTextureRenderTarget2D* butterfly_param = butterfly;
	UTextureRenderTarget2D* output_param    = output;

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

void ShaderModelsModule::ResetGPUBoat(UTextureRenderTarget2D* input_output) {

 	TShaderMapRef<GPUBoatShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	ENQUEUE_RENDER_COMMAND()([shader, input_output](FRHICommandListImmediate& RHI_cmd_list) {
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
	UTextureRenderTarget2D* input_output,
	UTextureRenderTarget2D* readback_texture,
	AActor* camera_target) {

	TRefCountPtr<FRDGPooledBuffer> submerged_triangles_buffer;
	{
		TShaderMapRef<SubmergedTrianglesShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		ENQUEUE_RENDER_COMMAND(shader)(
			[shader, collision_mesh, elevation_texture, input_output, &submerged_triangles_buffer](FRHICommandListImmediate& RHI_cmd_list) {
				shader->BuildAndExecuteGraph(
					RHI_cmd_list,
					collision_mesh,
					elevation_texture,
					input_output,
					&submerged_triangles_buffer
				);
			}); 
		// TODO: Maybe not needed? In Vulkan we would use a semaphore here instead of a fence.
		FRenderCommandFence fence;
		fence.BeginFence();
		fence.Wait();
	}

	TArray<FFloat16Color> data;
	{
		TShaderMapRef<GPUBoatShader> shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		ENQUEUE_RENDER_COMMAND(shader)(
			[shader, speed_input, velocity_input, elevation_texture, submerged_triangles_buffer, input_output, readback_texture, camera_target, &data](FRHICommandListImmediate& RHI_cmd_list) {
				shader->BuildAndExecuteGraph(
					RHI_cmd_list,
					speed_input,
					velocity_input,
					elevation_texture,
					submerged_triangles_buffer,
					input_output,
					readback_texture,
					camera_target ? (&data) : nullptr
				);
			}); 
	}

	if (camera_target) {
		FRenderCommandFence fence;
		fence.BeginFence();
		fence.Wait();

		FVector pos = FVector(data[0].R, data[0].G, data[0].B);
		FQuat rot   = FQuat(data[1].R, data[1].G, data[1].B, data[1].A);

		// UE_LOG(LogTemp, Warning, TEXT("Debug output: (%f, %f, %f)"), pos.X, pos.Y, pos.Z);
		// UE_LOG(LogTemp, Warning, TEXT("Debug output: (%f, %f, %f, %f)"), rot.X, rot.Y, rot.Z, rot.W);
		UE_LOG(LogTemp, Warning, TEXT("Debug output: (%f, %f, %f, %f)"), data[2].R.GetFloat(), data[2].G.GetFloat(), data[2].B.GetFloat(), data[2].A.GetFloat());

		camera_target->SetActorLocation(METERS_TO_UNREAL_UNITS * pos);
		camera_target->SetActorRotation(rot, ETeleportType::None);
	}
}

IMPLEMENT_MODULE(ShaderModelsModule, ShaderModels);