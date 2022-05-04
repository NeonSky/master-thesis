#include "FFTPerformer.h"


AFFTPerformer::AFFTPerformer() {
    // Configure Tick() to be called every frame.
    PrimaryActorTick.bCanEverTick = true;
}

void AFFTPerformer::BeginPlay() {
	Super::BeginPlay();

	m_shader_models_module = FModuleManager::LoadModuleChecked<ShaderModelsModule>("ShaderModels");

    if (this->ffts_per_frame > 0) {
        UE_LOG(LogTemp, Warning, TEXT("Performing %i FFTs per frame"), this->ffts_per_frame);
    }
}

void AFFTPerformer::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

    for (int i = 0; i < this->ffts_per_frame; i++) {
        m_shader_models_module.FFT(this->butterfly_rtt, this->target_rtt);
    }
}