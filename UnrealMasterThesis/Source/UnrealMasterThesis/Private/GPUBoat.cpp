#include "GPUBoat.h"

#include "Globals/StatelessHelpers.h"

#include "Kismet/GameplayStatics.h"

AGPUBoat::AGPUBoat() {}

void AGPUBoat::BeginPlay() {
	  Super::BeginPlay();

    m_shader_models_module.ResetGPUBoat(boat_rtt);
}

void AGPUBoat::Update(UpdatePayload update_payload, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) {

    if (IsHidden()) {
        SetActorHiddenInGame(false);
    }

    FVector2D velocity_input = use_p2_inputs ? update_payload.velocity_input2 : update_payload.velocity_input;

    m_shader_models_module.UpdateGPUBoat(
        update_payload.speed_input,
        velocity_input,
        collision_mesh,
        elevation_rtt,
        ewave_rtts.eWaveH,
        boat_rtt,
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