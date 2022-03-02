// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InputPawn.h"

#include "IBoat.generated.h"

// See: https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/GameplayArchitecture/Interfaces/
UINTERFACE(MinimalAPI, Blueprintable)
class UBoatInterface : public UInterface {
    GENERATED_BODY()
};

class IBoatInterface {
    GENERATED_BODY()

public:
    virtual void Update(UpdatePayload update_payload);
};
