#include "CPUBoat.h"

#include "Kismet/GameplayStatics.h"

ACPUBoat::ACPUBoat() {}

void ACPUBoat::BeginPlay() {
    Super::BeginPlay();

    input_pawn->on_fixed_update.AddUObject<ACPUBoat>(this, &ACPUBoat::Update);

    m_shader_models_module.ResetGPUBoat(boat_rtt);

    for (int i = 0; i < artificial_frame_delay+1; i++) {
        UCanvasRenderTarget2D* rtt = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this->GetWorld(), UCanvasRenderTarget2D::StaticClass(), elevation_rtt->SizeX, elevation_rtt->SizeY);
        rtt->UpdateResourceImmediate(true); // clear values
        m_readback_queue.push(rtt);
    }
    m_requested_elevations_on_frame = 0;
    m_cur_frame = 0;
}

void ACPUBoat::UpdateReadbackQueue() {

    if (m_cur_frame - m_requested_elevations_on_frame >= artificial_frame_skip) {
        m_requested_elevations_on_frame = m_cur_frame;

        // Time for new fetch
        ENQUEUE_RENDER_COMMAND()(
            [this](FRHICommandListImmediate& RHI_cmd_list) {
                RHI_cmd_list.CopyToResolveTarget(
                    elevation_rtt->GetRenderTargetResource()->TextureRHI,
                    m_readback_queue.front()->GetRenderTargetResource()->TextureRHI,
                    FResolveParams()
                );
            });

        // NOTE: safety measure
		FRenderCommandFence fence;
		fence.BeginFence();
		fence.Wait();

        // Requeue latest fetch
        UCanvasRenderTarget2D* rtt = m_readback_queue.front();
        m_readback_queue.pop();
        m_readback_queue.push(rtt);
    }

}

void ACPUBoat::Update(UpdatePayload update_payload) {

    UE_LOG(LogTemp, Warning, TEXT("11:48"));

    UpdateReadbackQueue();

    m_shader_models_module.UpdateGPUBoat(
        update_payload.speed_input,
        update_payload.velocity_input,
        collision_mesh,
        m_readback_queue.front(),
        boat_rtt,
        readback_rtt,
        this);

    m_cur_frame++;
}