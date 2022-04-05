// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>

#include "InputPawn.h"
#include "RenderGraphResources.h"

#include "IBoat.generated.h"

USTRUCT(BlueprintType)
struct FeWaveRTTs {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
    UTextureRenderTarget2D* eWaveHV;

	UPROPERTY(EditAnywhere)
    UTextureRenderTarget2D* eWaveHV_prev;
};

// See: https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/GameplayArchitecture/Interfaces/
UINTERFACE(MinimalAPI, Blueprintable)
class UBoatInterface : public UInterface {
    GENERATED_BODY()
};

class IBoatInterface {
    GENERATED_BODY()

public:
    virtual void Update(UpdatePayload update_payload, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) = 0;
    virtual UTextureRenderTarget2D* GetBoatRTT() = 0;
    virtual FeWaveRTTs GeteWaveRTTs() = 0;
	virtual FVector2D WorldPosition() = 0;
};
