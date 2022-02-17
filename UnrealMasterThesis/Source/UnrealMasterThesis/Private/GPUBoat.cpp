#include "GPUBoat.h"

#include "Kismet/GameplayStatics.h"

AGPUBoat::AGPUBoat() {}

void AGPUBoat::BeginPlay() {
	  Super::BeginPlay();

    input_pawn->on_fixed_update.AddUObject<AGPUBoat>(this, &AGPUBoat::Update);

    m_shader_models_module.ResetGPUBoat(boat_rtt);
}

void AGPUBoat::Update(UpdatePayload update_payload) {

    m_shader_models_module.UpdateGPUBoat(
        update_payload.speed_input,
        update_payload.velocity_input,
        elevation_rtt,
        boat_rtt,
        readback_rtt,
        camera_follow ? camera_target : nullptr);
}