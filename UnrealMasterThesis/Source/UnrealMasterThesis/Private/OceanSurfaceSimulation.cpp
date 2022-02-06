// Fill out your copyright notice in the Description page of Project Settings.

#include "OceanSurfaceSimulation.h"
#include "Globals/StatelessHelpers.h"

#include "ImageUtils.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"

// Having this in the editor would be nice, but we need to use CreateDefaultSubobject (it seems).
// This function may only be called from the constructor, which doesn't have access to the initialized editor properties.
// The NewObject function could potentially work, but it does not appear to give visible results in our case.
const int TILES_COUNT = 49; // Should be 1 or higher


AOceanSurfaceSimulation::AOceanSurfaceSimulation() {
	UE_LOG(LogTemp, Warning, TEXT("AOceanSurfaceSimulation::AOceanSurfaceSimulation()"));

	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize the tiles as empty procedural meshes.
	this->tile_meshes.SetNum(TILES_COUNT);
	for (int i = 0; i < TILES_COUNT; i++) {
		FName name = FName(*FString::Printf(TEXT("Ocean tile %i"), i+1));
		this->tile_meshes[i] = CreateDefaultSubobject<UProceduralMeshComponent>(name);
	}
}

void AOceanSurfaceSimulation::BeginPlay() {
	Super::BeginPlay();

	// Check that RTTs have correct dimensions.
	// The alternative of resizing RTTs during runtime doesn't appear to be a great idea: https://answers.unrealengine.com/questions/177345/changing-rendertexture-size-at-runtime-dramaticall.html?sort=oldest
	this->N = pow(2, this->butterfly_rtt->SizeX); // ensures power of 2

	if (this->butterfly_rtt->SizeY != this->N) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("this->butterfly_rtt->SizeY != this->N, where N = %i"), this->N));
	}

	if (this->spectrum_x_rtt->SizeX != this->spectrum_x_rtt->SizeY || this->spectrum_x_rtt->SizeY != this->N) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Wrong dimensions for spectrum_x_rtt"));
	}

	if (this->spectrum_y_rtt->SizeX != this->spectrum_y_rtt->SizeY || this->spectrum_y_rtt->SizeY != this->N) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Wrong dimensions for spectrum_y_rtt"));
	}

	if (this->spectrum_z_rtt->SizeX != this->spectrum_z_rtt->SizeY || this->spectrum_z_rtt->SizeY != this->N) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Wrong dimensions for spectrum_z_rtt"));
	}

	create_mesh();

	for (int i = 0; i < TILES_COUNT; i++) {
		this->tile_meshes[i]->SetMaterial(0, this->material);
	}

	// ----
}

void AOceanSurfaceSimulation::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	this->update_mesh(DeltaTime);
}

void AOceanSurfaceSimulation::create_mesh() {

	float spatial_size = L * METERS_TO_UNREAL_UNITS;
	float z_pos = 0.0;

	// Vertices
	TArray<FVector> vertices;

	// UV-coordinates
	TArray<FVector2D> uv0;
	TArray<FVector2D> uv1;
	TArray<FVector2D> uv2;
	TArray<FVector2D> uv3;

	// [0, N-1] represent the ocean patch, while the Nth entry (along each direction) starts the repeat.
	for (int32 x = 0; x <= this->N; x++) {
		for (int32 y = 0; y <= this->N; y++) {

			float x_pos = spatial_size * (float) x / (float) this->N - 0.5 * spatial_size;
			float y_pos = spatial_size * (float) y / (float) this->N - 0.5 * spatial_size;
			vertices.Add(FVector(x_pos, y_pos, z_pos));

			float u = (float) x / (float) this->N;
			float v = (float) y / (float) this->N;
			uv0.Add(FVector2D(u, v));
		}
	}

	m_shader_models_module = FModuleManager::LoadModuleChecked<ShaderModelsModule>("ShaderModels");
	m_shader_models_module.GenerateButterflyTexture(this->butterfly_rtt);

	// Becomes the directional spreading function chosen from the editor.
	std::function<float(FVector2D, FVector2D)> d = [this](FVector2D k_vec, FVector2D wind_direction) {

		float k = k_vec.Size();
		float g = GRAVITY;

		float omega = sqrt(k * g);

		// Angle of wave vector
		float theta = atan2(k_vec.Y, k_vec.X);

		// Angle of wind (assumed to be 0, i.e. positive x-axis, in most literature)
		float theta_p = atan2(wind_direction.Y, wind_direction.X); // The peak is equal to the wind direction

		// Angle of wave vector relative to the wind (Horvath equation 41)
		theta = theta - theta_p;

		float res = 0.0f;

		if (directional_spreading_function == DirectionalSpreadingType::Uniform) {
			res = UniformDirectionalSpectrum();
		}
		else if (directional_spreading_function == DirectionalSpreadingType::DonelanBanner) {
			res = DonelanBannerDirectionalSpectrum(theta, omega, donelan_banner_settings);
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("Unknown enum!"));
		}

		return FMath::Clamp(res, 0.0f, 1.0f);
	};

	// Becomes the wave spectrum chosen from the editor.
	std::function<float(FVector2D)> w = [this, d](FVector2D k_vec) {

		float res = 0.0f;

		if (wave_spectrum == WaveSpectrumType::Phillips) {
			phillips_settings.wind_direction.Normalize();
			res = PhillipsWaveSpectrum(k_vec, phillips_settings) * d(k_vec, phillips_settings.wind_direction);
		}
		else if (wave_spectrum == WaveSpectrumType::Jonswap) {
			jonswap_settings.wind_direction.Normalize();
			res = JonswapWaveSpectrum(k_vec, jonswap_settings) * d(k_vec, jonswap_settings.wind_direction);
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("Unknown enum!"));
		}

		return pow(amplitude_scaler, 2.0f) * res; // We square the scalar since the spectrum represents amplitude squared
	};

	m_shader_models_module.Buildh0Textures(this->N, this->L, w);

	// Triangles
	// We use (N+1)^2 vertices instead of N^2 in order to produce seamless tiling (diplacement of vertex N+1 will equal that of vertex 0, in a given horizontal axis).
	TArray<int32> triangles;
	UKismetProceduralMeshLibrary::CreateGridMeshTriangles(N + 1, N + 1, false, triangles);

	// Normals and Tangents
	TArray<FVector> normals;
	TArray<FProcMeshTangent> tangents;

	int32 section_index = 0;
	TArray<FColor> vertex_colors;
	bool create_collision = false;

	// Outward spiral movement. Credit: https://stackoverflow.com/a/14010215/8418261
	int layer = 1;
	int leg = 0;
	int x = 0;
	int y = 0;
	auto go_next = [&]() {
			switch (leg) {
        case 0: ++x; if(x  == layer)  ++leg;                break;
        case 1: ++y; if(y  == layer)  ++leg;                break;
        case 2: --x; if(-x == layer)  ++leg;                break;
        case 3: --y; if(-y == layer){ leg = 0; ++layer; }   break;
			}
	};

	TArray<FVector> offset_vertices = vertices;

	// Place tiles in an outward spiral pattern.
	for (int t = 0; t < TILES_COUNT; t++) {

		for (int32 u = 0; u <= this->N; u++) {
			for (int32 v = 0; v <= this->N; v++) {
				int i = u * (this->N+1) + v;
				offset_vertices[i] = vertices[i] + FVector(spatial_size * x, spatial_size * y, 0.0);
			}
		}

		this->tile_meshes[t]->CreateMeshSection(section_index, offset_vertices, triangles, normals, uv0, uv1, uv2, uv3, vertex_colors, tangents, create_collision);

		go_next();
	}
}

void AOceanSurfaceSimulation::update_mesh(float dt) {
	float realtimeSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());

	//m_shader_models_module.ComputeFourierComponents(realtimeSeconds, L, this->spectrum_x_rtt, this->spectrum_y_rtt, this->spectrum_z_rtt);

	//m_shader_models_module.FFT(this->butterfly_rtt, this->spectrum_x_rtt);
	//m_shader_models_module.FFT(this->butterfly_rtt, this->spectrum_y_rtt);
	//m_shader_models_module.FFT(this->butterfly_rtt, this->spectrum_z_rtt);


	
		// Add height addition texture to the height field
	static bool flip = true;
	static bool first = true;

	static float last_ran = realtimeSeconds;
	static int counter = 0;
	//if (realtimeSeconds - last_ran >= 2.0f && counter < 2) {
		last_ran = realtimeSeconds;
		
		if (first) {
			// Note: parameter 2 is overridden in the Add shader, the test texture is passed as parameter 2 to the shader
		    // Note: currently does not even add, just writes the test data to a the render target from parameter 3. 
			m_shader_models_module.ComputeAdd(this->eWave_addition_rtt, this->eWave_addition_texture, this->eWave_addition_rtt);
			
			// TODO: remove the scale parameter
			//UE_LOG(LogTemp, Error, TEXT("ADD"));
		//}
		//else {
			//m_shader_models_module.FFT(this->butterfly_rtt, this->eWave_addition_rtt, 1.0f);
			//m_shader_models_module.ComputeScale(this->eWave_addition_rtt, this->eWave_addition_rtt, 1, -1);
			
			//m_shader_models_module.FFT2(this->butterfly_rtt, this->eWave_addition_rtt, 1.0 / (N * N)); // TODO: remove the scale parameter

			//float scale = 1.0 / (N * N);
			//m_shader_models_module.ComputeScale(this->eWave_addition_rtt, this->eWave_addition_rtt, scale, -1.0 * scale);
			
			first = false;
			//UE_LOG(LogTemp, Error, TEXT("FFT"));
		}
		float scale = 1 / (N * N);
		m_shader_models_module.FFT_Forward(this->butterfly_rtt, this->eWave_addition_rtt, 0);
		//m_shader_models_module.ComputeScale(this->eWave_addition_rtt, this->eWave_addition_rtt, 1.0, -1.0);

		m_shader_models_module.ComputeeWave(0.016, L, this->ewave_h_rtt, this->ewave_hPrev_rtt, this->ewave_v_rtt, this->ewave_vPrev_rtt);

		m_shader_models_module.FFT_Forward(this->butterfly_rtt, this->eWave_addition_rtt, 0);
		m_shader_models_module.ComputeScale(this->eWave_addition_rtt, this->eWave_addition_rtt, 1.0 / 16, -1.0 * scale);
		
		
		
		flip = !flip;
		counter++;
	//}


}