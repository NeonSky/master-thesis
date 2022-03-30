// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InputPawn.h"
#include "RenderGraphResources.h"

#include "IBoat.generated.h"

// See: https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/GameplayArchitecture/Interfaces/
UINTERFACE(MinimalAPI, Blueprintable)
class UBoatInterface : public UInterface {
    GENERATED_BODY()
};

class IBoatInterface {
    GENERATED_BODY()

public:
    virtual void Update(UpdatePayload update_payload) = 0;
    virtual UTextureRenderTarget2D* GetBoatRTT() = 0;
    virtual TRefCountPtr<FRDGPooledBuffer> GetSubmergedTriangles() = 0;
    virtual FVector getPosition() = 0;
    virtual FQuat getRotation() = 0;
};
