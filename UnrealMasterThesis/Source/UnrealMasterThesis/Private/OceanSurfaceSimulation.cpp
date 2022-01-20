// Fill out your copyright notice in the Description page of Project Settings.

#include "OceanSurfaceSimulation.h"
#include "Globals/StatelessHelpers.h"

#include "ImageUtils.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"

static const float METERS_TO_UNREAL_UNITS = 100;

AOceanSurfaceSimulation::AOceanSurfaceSimulation() {
	UE_LOG(LogTemp, Warning, TEXT("AOceanSurfaceSimulation::AOceanSurfaceSimulation()"));

	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize the mesh as an empty procedural mesh.
	this->mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Generated Ocean Surface"));
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

	this->mesh->SetMaterial(0, this->material);
}

void AOceanSurfaceSimulation::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	this->update_mesh();
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

	this->mesh->CreateMeshSection(section_index, vertices, triangles, normals, uv0, uv1, uv2, uv3, vertex_colors, tangents, create_collision);
}

void AOceanSurfaceSimulation::update_mesh() {
	float realtimeSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());

	m_shader_models_module.ComputeFourierComponents(realtimeSeconds, L, this->spectrum_x_rtt, this->spectrum_y_rtt, this->spectrum_z_rtt);

	m_shader_models_module.FFT(this->butterfly_rtt, this->spectrum_x_rtt);
	m_shader_models_module.FFT(this->butterfly_rtt, this->spectrum_y_rtt);
	m_shader_models_module.FFT(this->butterfly_rtt, this->spectrum_z_rtt);
}