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
	// Initialize the tiles as empty procedural meshes.
	this->tile_meshes.SetNum(TILES_COUNT);
	for (int i = 0; i < TILES_COUNT; i++) {
		FName name = FName(*FString::Printf(TEXT("Ocean tile %i"), i+1));
		this->tile_meshes[i] = CreateDefaultSubobject<UProceduralMeshComponent>(name);
	}

	data_collector = CreateDefaultSubobject<UDataCollector>(TEXT("Data Collector"));
	data_collector->SetupAttachment(RootComponent);
}

void AOceanSurfaceSimulation::BeginPlay() {
	Super::BeginPlay();

	this->time = 0.0f;
	this->frame = 0;
	this->average_cpu_cost = 0.0f;

	if (oceanSeed == 0) {
		oceanSeed = FMath::RandRange(1, 10000);
	}

	// Check that RTTs have correct dimensions.
	// The alternative of resizing RTTs during runtime doesn't appear to be a great idea: https://answers.unrealengine.com/questions/177345/changing-rendertexture-size-at-runtime-dramaticall.html?sort=oldest
	this->N = pow(2, this->butterfly_rtt->SizeX); // ensures power of 2

	if (this->butterfly_rtt->SizeY != this->N) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("this->butterfly_rtt->SizeY != this->N, where N = %i"), this->N));
	}

	if (this->spectrum_y_rtt->SizeX != this->spectrum_y_rtt->SizeY || this->spectrum_y_rtt->SizeY != this->N) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Wrong dimensions for spectrum_y_rtt"));
	}

	if (this->spectrum_xz_rtt->SizeX != this->spectrum_xz_rtt->SizeY || this->spectrum_xz_rtt->SizeY != this->N) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Wrong dimensions for spectrum_xz_rtt"));
	}

	create_mesh();

	for (int i = 0; i < TILES_COUNT; i++) {
		this->tile_meshes[i]->SetMaterial(0, this->material);
	}

	for (auto boat : boats) {
		if (boat) {
			FeWaveRTTs ewave_rtts = boat->GeteWaveRTTs();
			m_shader_models_module.Clear(ewave_rtts.obstruction, FVector4(0.0, 0.0, 0.0, 1.0));
			m_shader_models_module.Clear(ewave_rtts.eWaveHV, FVector4(0.0, 0.0, 0.0, 1.0));
			m_shader_models_module.Clear(ewave_rtts.eWaveHV_prev, FVector4(0.0, 0.0, 0.0, 1.0));
		}
	}
	m_shader_models_module.Clear(spectrum_xz_rtt, FVector4(0.0, 0.0, 0.0, 1.0));
	m_shader_models_module.Clear(spectrum_y_rtt, FVector4(0.0, 0.0, 0.0, 1.0));

	input_pawn->on_fixed_update.AddUObject<AOceanSurfaceSimulation>(this, &AOceanSurfaceSimulation::update);
	input_pawn->playBackInputSequence = data_collection_settings.shouldPlayBackInputSequence;

	data_collector->shaderModule = &m_shader_models_module;
	data_collector->data_collection_settings = data_collection_settings;

	data_collector->serialization_rtt = serialization_rtt;
	TArray<float> delay_dist = data_collector->readOrganicDistributionJSON();

	if (boats.Num() > 0 && boats[0] != nullptr) {
		boats[0]->setDist(delay_dist, oceanSeed); 
		data_collector->eWave_hv_rtt = boats[0]->GeteWaveRTTs().eWaveHV; // eWave measurements assume the boat is on index 0. TODO: possibly allow for 2 boats.
	}
	
	for (auto boat : boats) {
		data_collector->boats.Add(boat);
		data_collector->boatPositions.Add(TArray<FVector>());
		data_collector->boatPositions.Last().Reserve(data_collection_settings.framesToRecord);
		if (data_collection_settings.shouldPlayBackInputSequence) {
			data_collector->readInputJSON(input_pawn->preRecordedInputSequence);
		}
	}
}

void AOceanSurfaceSimulation::update(UpdatePayload update_payload) {
	if (!should_simulate) {
		return;
	}

	auto timer_start = std::chrono::high_resolution_clock::now();

	if (this->cur_cpu_cost != 0.0f) {
		int prev_frame = frame-1;
		if (prev_frame == 0) {
			average_cpu_cost = this->cur_cpu_cost;
		} else {
			average_cpu_cost = (this->cur_cpu_cost + prev_frame * average_cpu_cost) / (prev_frame+1);

			if (prev_frame % 60 == 0) {
				// UE_LOG(LogTemp, Warning, TEXT("CPU cost: %f ms"), average_cpu_cost);
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("CPU cost: %f ms"), average_cpu_cost));
			}
		}
	}

	this->cur_cpu_cost = 0.0f;

	time += fixed_dt;
	this->m_submerged_triangles_buffers.SetNum(boats.Num());

	int n_valid_boats = 0;
	for (auto boat : boats) {
		if (boat) {
			n_valid_boats++;
		}
	}

	static int counter = 0;
	static bool counted[100] = { false }; // one for each boat

	if (n_valid_boats == 0) {
		this->update_mesh(fixed_dt);
	}
	else {
		// Create one callback per loop that stores a different index.
		for (int i = 0; i < boats.Num(); i++) {
			auto boat = boats[i];
			// Allow "None", i.e. nullptr, to be assigned for boats in the editor.
			if (boat) {

				auto callback = [n_valid_boats, i, this](TRefCountPtr<FRDGPooledBuffer> submerged_triangles_buffer) {

					if (!submerged_triangles_buffer.IsValid()) {
						UE_LOG(LogTemp, Warning, TEXT("This shouldn't be possible"));
						return;
					}

					if (!counted[i]) {
						counted[i] = true;
						counter++;
					}

					this->m_submerged_triangles_buffers[i] = submerged_triangles_buffer;

					if (counter == n_valid_boats) {
						memset(counted, false, sizeof(counted));
						counter = 0;
						this->update_mesh(fixed_dt);
					}
				};

				boat->Update(update_payload, callback);
			}
		}
	}
	data_collector->update(update_payload);

	if (measure_cpu_cost) {
		auto timer_end = std::chrono::high_resolution_clock::now();
		auto timer_duration = std::chrono::duration_cast<std::chrono::microseconds>(timer_end - timer_start);
		float ms_timer_duration = ((float) timer_duration.count()) / 1000.0f;
		this->cur_cpu_cost += ms_timer_duration;
	}
}

TArray<float> AOceanSurfaceSimulation::sample_elevation_points(TArray<FVector2D> sample_points, bool mock_async_readback) {

	TArray<float> elevation_output;

	TArray<UTextureRenderTarget2D*> wake_rtts;
	TArray<FVector2D> ws_boat_coords;

	for (auto boat : boats) {
		if (boat) {
			wake_rtts.Add(boat->GeteWaveRTTs().eWaveHV);
			FVector boatPos = boat->WorldPosition3D();
			ws_boat_coords.Add(FVector2D(boatPos.X, boatPos.Y));
		}
	}

	m_shader_models_module.SampleElevationPoints(
		this->spectrum_y_rtt,
		wake_rtts,
		ws_boat_coords,
		sample_points,
		&elevation_output,
		mock_async_readback
	);

	// Force the output to be ready since UE will not allow the render thread to get 2 frames behind the game thread anyway. 
	if (mock_async_readback) {
		elevation_output.SetNum(sample_points.Num());
	} else {
		FRenderCommandFence fence;
		fence.BeginFence();
		fence.Wait();
	}

	return elevation_output;
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

	m_shader_models_module.Buildh0Textures(this->N, this->L, w, oceanSeed);

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

	auto timer_start = std::chrono::high_resolution_clock::now();

	// Update non-interactive ocean.
	m_shader_models_module.ComputeFourierComponents(time, this->spectrum_y_rtt, this->spectrum_xz_rtt);

	m_shader_models_module.FFT(this->butterfly_rtt, this->spectrum_y_rtt);
	m_shader_models_module.FFT(this->butterfly_rtt, this->spectrum_xz_rtt);

	if (should_update_wakes) {
		// Update interactive wake simulation on top of the non-interactive ocean
		float ewave_scale = 1.0f / ((float)N * (float)N);

		for (int i = 0; i < boats.Num(); i++) {
			auto boat = boats[i];
			if (boat) {
				UTextureRenderTarget2D* boat_rtt = boat->GetBoatRTT();
				TRefCountPtr<FRDGPooledBuffer> submerged_triangles = m_submerged_triangles_buffers[i];

				FeWaveRTTs ewave_rtts = boat->GeteWaveRTTs();

				UTextureRenderTarget2D* src = ewave_rtts.eWaveHV;
				UTextureRenderTarget2D* dst = ewave_rtts.eWaveHV_prev;

				{ // TODO: Maybe no longer needed
					ENQUEUE_RENDER_COMMAND(void)(
						[src, dst](FRHICommandListImmediate& RHI_cmd_list) {
							RHI_cmd_list.CopyToResolveTarget(
								src->GetRenderTargetResource()->GetRenderTargetTexture(),
								dst->GetRenderTargetResource()->GetRenderTargetTexture(),
								FResolveParams()
							);
						});
				}

				m_shader_models_module.ComputeObstruction(boat_rtt, submerged_triangles, ewave_rtts.obstruction, ewave_rtts.eWaveHV, ewave_rtts.eWaveHV_prev, 2);
				m_shader_models_module.ComputeObstruction(boat_rtt, submerged_triangles, ewave_rtts.obstruction, ewave_rtts.eWaveHV, ewave_rtts.eWaveHV_prev, 1);
				m_shader_models_module.FFT_Forward(this->butterfly_rtt, ewave_rtts.eWaveHV); // https://www.dsprelated.com/showarticle/800.php, inverse fft article.

				{ // Copy prevents race condition in eWave.usf
					ENQUEUE_RENDER_COMMAND(void)(
						[src, dst](FRHICommandListImmediate& RHI_cmd_list) {
							RHI_cmd_list.CopyToResolveTarget(
								src->GetRenderTargetResource()->GetRenderTargetTexture(),
								dst->GetRenderTargetResource()->GetRenderTargetTexture(),
								FResolveParams()
							);
						});
				}

				m_shader_models_module.ComputeeWave(dt, ewave_rtts.eWaveHV, ewave_rtts.eWaveHV_prev);
				m_shader_models_module.FFT(this->butterfly_rtt, ewave_rtts.eWaveHV, 0);
				m_shader_models_module.ComputeScale(ewave_rtts.eWaveHV, ewave_rtts.eWaveHV_prev, ewave_scale);
				m_shader_models_module.ComputeObstruction(boat_rtt, submerged_triangles, ewave_rtts.obstruction, ewave_rtts.eWaveHV, ewave_rtts.eWaveHV_prev, 0);
			}
		}
	}

	if (measure_cpu_cost) {
		auto timer_end = std::chrono::high_resolution_clock::now();
		auto timer_duration = std::chrono::duration_cast<std::chrono::microseconds>(timer_end - timer_start);
		float ms_timer_duration = ((float) timer_duration.count()) / 1000.0f;
		this->cur_cpu_cost += ms_timer_duration;
	}

	this->frame++;
}