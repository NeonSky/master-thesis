#include "GPUBoat.h"

#include "Globals/StatelessHelpers.h"

#include "Kismet/GameplayStatics.h"

// GPU boats will only affect other GPU boats, making it possible to compare GPU and Artificial boat without interactions between them.
struct SharedState {
    TArray<UTextureRenderTarget2D*> boat_rtts;
    TArray<UTextureRenderTarget2D*> ewave_rtts;
};
static SharedState shared_state;

AGPUBoat::AGPUBoat() {}
AGPUBoat::~AGPUBoat() {
    shared_state = SharedState();
}

void AGPUBoat::BeginPlay() {
	  Super::BeginPlay();

    m_shader_models_module.ResetGPUBoat(boat_rtt);
}

void AGPUBoat::Update(UpdatePayload update_payload, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) {

    if (IsHidden()) {
        SetActorHiddenInGame(false);
        shared_state.boat_rtts.Push(this->boat_rtt);
        shared_state.ewave_rtts.Push(this->ewave_rtts.eWaveH);
    }

    FVector2D velocity_input = use_p2_inputs ? update_payload.velocity_input2 : update_payload.velocity_input;

    TArray<UTextureRenderTarget2D*> other_boat_textures;
    for (auto& rtt : shared_state.boat_rtts) {
        if (rtt != boat_rtt) {
            other_boat_textures.Push(rtt);
        }
    }

    m_shader_models_module.UpdateGPUBoat(
        update_payload.speed_input,
        velocity_input,
        collision_mesh,
        elevation_rtt,
        shared_state.ewave_rtts,
        boat_rtt,
        other_boat_textures,
        readback_rtt,
        camera_follow ? camera_target : nullptr,
        callback);

}

UTextureRenderTarget2D* AGPUBoat::GetBoatRTT() {
    return boat_rtt;
}

FeWaveRTTs AGPUBoat::GeteWaveRTTs() {
    return ewave_rtts;
}

FVector2D AGPUBoat::WorldPosition() {
  return FVector2D(camera_target->GetActorLocation().X, camera_target->GetActorLocation().Y) / METERS_TO_UNREAL_UNITS;
}