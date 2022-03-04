#include "CPUBoat.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"

ACPUBoat::ACPUBoat() {}

void ACPUBoat::BeginPlay() {
    Super::BeginPlay();

    m_shader_models_module.ResetGPUBoat(boat_rtt);

    for (int i = 0; i < artificial_frame_delay+1; i++) {
        m_readback_queue.push(readback_bank[i]);
    }
    m_requested_elevations_on_frame = 0;
    m_cur_frame = 0;
}

void ACPUBoat::UpdateReadbackQueue() {

    if (m_cur_frame - m_requested_elevations_on_frame >= artificial_frame_skip) {
        m_requested_elevations_on_frame = m_cur_frame;

        UTextureRenderTarget2D* src = elevation_rtt;
        UTextureRenderTarget2D* rtt = m_readback_queue.front();
        m_readback_queue.pop();

        FRenderCommandFence fence;
        ENQUEUE_RENDER_COMMAND(void)(
            [src, rtt](FRHICommandListImmediate& RHI_cmd_list) {
                RHI_cmd_list.CopyToResolveTarget(
                    src->GetRenderTargetResource()->GetRenderTargetTexture(),
                    rtt->GetRenderTargetResource()->GetRenderTargetTexture(),
                    FResolveParams()
                );
            });

        // NOTE: safety measure
        fence.BeginFence();
        fence.Wait();

        // Requeue latest fetch
        m_readback_queue.push(rtt);
    }

}

void ACPUBoat::Update(UpdatePayload update_payload) {

    if (IsHidden()) {
        SetActorHiddenInGame(false);
    }

    UpdateReadbackQueue();

    m_shader_models_module.UpdateGPUBoat(
        update_payload.speed_input,
        update_payload.velocity_input,
        collision_mesh,
        m_readback_queue.front(),
        wake_rtt,
        boat_rtt,
        readback_rtt,
        m_submerged_triangles,
        this);

    m_cur_frame++;
}

UTextureRenderTarget2D* ACPUBoat::GetBoatRTT() {
    return boat_rtt;
}

TRefCountPtr<FRDGPooledBuffer> ACPUBoat::GetSubmergedTriangles() {
    return m_submerged_triangles;
}