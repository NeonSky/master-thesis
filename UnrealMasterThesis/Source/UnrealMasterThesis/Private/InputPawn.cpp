#include "InputPawn.h"
#include "Globals/StatelessHelpers.h"
#include "Kismet/GameplayStatics.h"

AInputPawn::AInputPawn() {
	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;
}

void AInputPawn::BeginPlay() {
	Super::BeginPlay();

    m_velocity_input = FVector2D(0.0f);
    m_speed_input = slow_speed;
}

void AInputPawn::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

    UpdatePayload payload;
    payload.speed_input = m_speed_input;
    payload.velocity_input = m_velocity_input;
    on_fixed_update.Broadcast(payload);

    APlayerController* controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    controller->SetViewTarget(camera_target, FViewTargetTransitionParams());
}

void AInputPawn::SetupPlayerInputComponent(class UInputComponent* inputComponent) {
  Super::SetupPlayerInputComponent(inputComponent);

  inputComponent->BindAction("Speed1", IE_Pressed, this, &AInputPawn::UseSlowSpeed);
  inputComponent->BindAction("Speed2", IE_Pressed, this, &AInputPawn::UseNormalSpeed);
  inputComponent->BindAction("Speed3", IE_Pressed, this, &AInputPawn::UseFastSpeed);

  inputComponent->BindAction("Speed1", IE_Released, this, &AInputPawn::ReleaseKey_Speed1);
  inputComponent->BindAction("Speed2", IE_Released, this, &AInputPawn::ReleaseKey_Speed2);
  inputComponent->BindAction("Speed3", IE_Released, this, &AInputPawn::ReleaseKey_Speed3);

  inputComponent->BindAxis("HorizontalAxis", this, &AInputPawn::HorizontalAxis);
  inputComponent->BindAxis("VerticalAxis", this, &AInputPawn::VerticalAxis);
}

InputState AInputPawn::getInputState() {
    return currentState;
}

void AInputPawn::UseSlowSpeed()   { 
    m_speed_input = slow_speed; 
    currentState.speed_1 = 1; 
}

void AInputPawn::UseNormalSpeed() { 
    m_speed_input = normal_speed; 
    currentState.speed_2 = 1; 
}

void AInputPawn::UseFastSpeed() { 
    m_speed_input = fast_speed;  
    currentState.speed_3 = 1;
}

void AInputPawn::ReleaseKey_Speed1() { 
    currentState.speed_1 = 0; 
}
void AInputPawn::ReleaseKey_Speed2() { 
    currentState.speed_2 = 0; 
}
void AInputPawn::ReleaseKey_Speed3() { 
    currentState.speed_3 = 0; 
}

void AInputPawn::HorizontalAxis(float input) {
  m_velocity_input.X = input;
  currentState.horizontal = (input > 0) ? (1) : ((input < 0) ? (-1) : (0));
}

void AInputPawn::VerticalAxis(float input) {
  m_velocity_input.Y = input;
  currentState.vertical = (input > 0) ? (1) : ((input < 0) ? (-1) : (0));
}
