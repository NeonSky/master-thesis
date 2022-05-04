// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderModels.h"

#include "FFTPerformer.generated.h"

UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API AFFTPerformer : public AActor {
	GENERATED_BODY()
	
public:	
	AFFTPerformer();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* butterfly_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* target_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int ffts_per_frame;

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module

};
