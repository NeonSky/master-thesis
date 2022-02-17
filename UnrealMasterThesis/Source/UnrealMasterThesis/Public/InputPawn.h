// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InputPawn.generated.h"

struct UpdatePayload {
    float speed_input;
    FVector2D velocity_input;
};

// DECLARE_DELEGATE_OneParam(FOnFixedUpdate, UpdatePayload);
DECLARE_EVENT_OneParam(AInputPawn, FOnFixedUpdate, UpdatePayload)

UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API AInputPawn : public APawn {
	GENERATED_BODY()
	
public:	
	AInputPawn();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

    FOnFixedUpdate on_fixed_update;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AActor* camera_target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float slow_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float normal_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float fast_speed;

	FVector2D m_velocity_input;
	float m_speed_input;

	void UseSlowSpeed();
	void UseNormalSpeed();
	void UseFastSpeed();

	void HorizontalAxis(float input);
	void VerticalAxis(float input);
};