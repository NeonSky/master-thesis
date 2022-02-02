// Fill out your copyright notice in the Description page of Project Settings.

#include "Boat.h"
#include "Globals/StatelessHelpers.h"

#include "Engine/TextureRenderTarget2D.h"

DECLARE_CYCLE_STAT(TEXT("CustomDebug ~ ReadElevations"), STAT_ReadElevations, STATGROUP_CustomDebug);

float prev_time = 0.0;
int frame = 0;
int prev_frame = 0;

ABoat::ABoat() {
	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;
}

void ABoat::BeginPlay() {
	Super::BeginPlay();

  m_velocity_input = FVector2D(0.0f);
  m_speed_input = normal_speed;
  m_has_requested_elevations = false;
}

void ABoat::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

  ReadElevations();
  UpdateElevations();
  UpdateTransform(DeltaTime);

  frame++;
}

void ABoat::ReadElevations() {

  SCOPE_CYCLE_COUNTER(STAT_ReadElevations);

  // QUICK_SCOPE_CYCLE_COUNTER(TEXT("Stats::THIS_IS_A_TEST_SCOPE"));
  // QUICK_SCOPE_CYCLE_COUNTER(TEXT("Stats::Broadcast"));
  // QUICK_SCOPE_CYCLE_COUNTER(TEXT("ABoat::ReadElevations"));
  // FGenericPlatformProcess::Sleep(0.1f); // This was caught

 	if (m_has_requested_elevations) {
		return;
	}
  m_has_requested_elevations = true;

	UTextureRenderTarget2D* input = this->spectrum_y_rtt;
	TArray<FFloat16Color>* output = &this->m_elevation_data_scratch;
	FRenderCommandFence* fence = &m_elevation_read_fence;

	// frames_before = frames_after;
	// UE_LOG(LogTemp, Warning, TEXT("BEFORE: %f"), FPlatformTime::Seconds());

	// UE_LOG(LogTemp, Warning, TEXT("READ!"));
  prev_frame = frame;
	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
    [input, output, fence](FRHICommandListImmediate& RHI_cmd_list) {

      // SCOPE_CYCLE_COUNTER(STAT_ReadElevations);
      int32 N = 256;

			FRHITexture* rhi_tex = input->GetRenderTargetResource()->TextureRHI;

			FRHIResourceCreateInfo CreateInfo;
			FTexture2DRHIRef readback_tex = RHICreateTexture2D(
				N,
				N,
				PF_FloatRGBA,
				1,
				1,
				TexCreate_RenderTargetable,
				CreateInfo);

      // for (int i = 0; i < 100; i++) {
        RHI_cmd_list.CopyToResolveTarget(
          rhi_tex->GetTexture2D(),
          readback_tex,
          FResolveParams()
        );

        FReadSurfaceDataFlags read_flags(RCM_MinMax);
        read_flags.SetLinearToGamma(false);

        // ~ 6.15 ms
        // RHI_cmd_list.ReadSurfaceFloatData(
        //   readback_tex,
        //   FIntRect(0, 0, N, N),
        //   *output,
        //   read_flags
        // );

        // ~ 5.95 ms
        // RHI_cmd_list.ReadSurfaceFloatData(
        //   readback_tex,
        //   FIntRect(0, 0, N/2, N/2),
        //   *output,
        //   read_flags
        // );

        // ~ 6.3 ms ???
        // UE_LOG(LogTemp, Warning, TEXT("Weeee"));
        UE_LOG(LogTemp, Warning, TEXT("Reading 1/(8*8) of the texture"));
        RHI_cmd_list.ReadSurfaceFloatData(
          readback_tex,
          FIntRect(0, 0, N/8, N/8),
          *output,
          read_flags
        );
        UE_LOG(LogTemp, Warning, TEXT("Finished adding that command."));
      // }

      // UE_LOG(LogTemp, Warning, TEXT("Fence before: %i"), fence->IsFenceComplete());
      fence->BeginFence();
      // UE_LOG(LogTemp, Warning, TEXT("Fence after: %i"), fence->IsFenceComplete());

    }); 
}

void ABoat::UpdateElevations() {
  if (!m_has_requested_elevations || !m_elevation_read_fence.IsFenceComplete()) {
    return;
  }

  float time = FPlatformTime::Seconds();
	// UE_LOG(LogTemp, Warning, TEXT("Read complete: %f"), (time - prev_time) * 1000.0f);
	// UE_LOG(LogTemp, Warning, TEXT("Read complete after %i frames (%f ms)"), frame - prev_frame, (time - prev_time) * 1000.0f);
  prev_time = time;

  m_elevation_data = m_elevation_data_scratch;
  m_has_requested_elevations = false;
}

void ABoat::UpdateTransform(float DeltaTime) {
  if (m_velocity_input.IsNearlyZero(0.001f)) {
    return;
  }

  FVector forward = GetActorForwardVector();
  FVector right = GetActorRightVector();

  // The direction we want to move in
  FVector dir = (right * m_velocity_input.X + forward * m_velocity_input.Y).GetSafeNormal();

  // Apply rotation by interpolating from our previous direction towards the new one.
  FRotator cur_rot = FRotator(0.0f, GetActorRotation().Yaw, 0.0f);
  FRotator dir_rot = FRotator(0.0f, 180.0f * (atan2(dir.Y, dir.X) / PI), 0.0f);

  auto new_rot = FQuat::Slerp(cur_rot.Quaternion(), dir_rot.Quaternion(), rotation_speed);

  SetActorRotation(new_rot, ETeleportType::None);

  // The boat only accelerates back or forth.
  dir = forward * m_velocity_input.Y;

  // Apply movement step
  FVector pos = GetActorLocation();
  FVector delta_pos = METERS_TO_UNREAL_UNITS * m_speed_input * dir * DeltaTime;
  SetActorLocation(pos + delta_pos);
}

void ABoat::SetupPlayerInputComponent(class UInputComponent* InputComponent) {
  Super::SetupPlayerInputComponent(InputComponent);

  InputComponent->BindAction("Speed1", IE_Pressed, this, &ABoat::UseSlowSpeed);
  InputComponent->BindAction("Speed2", IE_Pressed, this, &ABoat::UseNormalSpeed);
  InputComponent->BindAction("Speed3", IE_Pressed, this, &ABoat::UseFastSpeed);

  InputComponent->BindAxis("HorizontalAxis", this, &ABoat::HorizontalAxis);
  InputComponent->BindAxis("VerticalAxis", this, &ABoat::VerticalAxis);
}

void ABoat::UseSlowSpeed()   { m_speed_input = slow_speed; }
void ABoat::UseNormalSpeed() { m_speed_input = normal_speed; }
void ABoat::UseFastSpeed()   { m_speed_input = fast_speed; }

void ABoat::HorizontalAxis(float input) {
  m_velocity_input.X = input;
}

void ABoat::VerticalAxis(float input) {
  m_velocity_input.Y = input;
}
