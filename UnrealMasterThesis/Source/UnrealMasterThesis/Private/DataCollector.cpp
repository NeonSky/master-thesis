// Fill out your copyright notice in the Description page of Project Settings.
#include "DataCollector.h"
#include "InputPawn.h"
#include "ImageUtils.h"
#include "ShaderModels.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/BufferArchive.h"
#include "Engine/TextureRenderTarget2D.h"




// Sets default values for this component's properties
UDataCollector::UDataCollector()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UDataCollector::BeginPlay()
{
	Super::BeginPlay();
	// readInputJSON(inputPawn->inputSequence); // TODO, this is currently called in oceansurfaceSimulations beginplay instead. inputPawn is nullptr here. 
}


// Called every frame
void UDataCollector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	frameNumber++;
	if (frameNumber >= framesToCollect) {
		if (shouldCollectInputStates) {
			saveInputToFile();
		}
		if (shouldCollectBoatData) {
			saveBoatDataToFile();
		}
		shouldCollectBoatData = false;
		shouldCollecteWaveTextures = false;
		shouldCollectInputStates = false;
		inputPawn->playBackInputSequence = false;
	}
	if (shouldCollectInputStates) {
		inputStates.Add(inputPawn->getInputState());
	}
	
	if (shouldCollecteWaveTextures) {
		TArray<float> h_rtt_r_channel_data;
		shaderModule->ComputeSerialization(eWave_h_rtt, serialization_rtt, h_rtt_r_channel_data);
		saveeWaveDataToFile(h_rtt_r_channel_data); // TODO save v texture
	}

	if (shouldCollectBoatData) {
		for (auto boat : boats) {
			if (boat) {
				FVector boatPos = boat->WorldPosition3D();
				boatPositions.Add(boat->WorldPosition3D());
				// boatOrientations.Add(boat->getOrientation()); // TODO
			}
			else {
				boatPositions.Add(FVector(0.0f, 0.0f, 0.0f));
			}
		}
	}

}

void UDataCollector::saveeWaveDataToFile(TArray<float>& data) {
	FString fname = *FString(TEXT("TempTestData/TestData") + FString::FromInt(frameNumber) + TEXT(".txt"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;

	FString textToSave = TEXT("Test\n");
	int i = 0;
	for (float f : data) {
		FString floatString = FString::SanitizeFloat(f);
		textToSave.Append(TEXT("r: ") + floatString + TEXT(", "));
		if (i >= 255) { 
			textToSave.Append(TEXT(" \n"));
			i = 0; 
		}
		i++;
	}
	FFileHelper::SaveStringToFile(textToSave, *AbsoluteFilePath);
}

void UDataCollector::saveInputToFile() {
	FString fname = *FString(TEXT("SavedInputData/TestData.json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
	TSharedRef<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> states;
	for (const auto& state : inputStates) {
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetNumberField("speed", state.speed);
		JsonObject->SetNumberField("horizontal", state.horizontal);
		JsonObject->SetNumberField("vertical", state.vertical);

		FString OutputString;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		states.Add(MakeShareable(new FJsonValueString(OutputString)));
	}
	JsonRootObject->SetArrayField("inputStates", states);

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonRootObject, Writer);

	FFileHelper::SaveStringToFile(OutputString, *AbsoluteFilePath);
}

void UDataCollector::saveBoatDataToFile() {
	FString fname = *FString(TEXT("SavedBoatData/TestData_Boat.json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
	TSharedRef<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> boatStates;
	for (const auto& pos : boatPositions) {
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetNumberField("position_x", pos.X);
		JsonObject->SetNumberField("position_y", pos.Y);
		JsonObject->SetNumberField("position_z", pos.Z);
		// JsonObject->SetNumberField("orientation_x", state.horizontal); // TODO

		FString OutputString;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		boatStates.Add(MakeShareable(new FJsonValueString(OutputString)));
	}
	JsonRootObject->SetArrayField("boatOrientations", boatStates);

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonRootObject, Writer);

	FFileHelper::SaveStringToFile(OutputString, *AbsoluteFilePath);
}

void UDataCollector::readInputJSON(TArray<InputState>& inputSequence) {
	FString fname = *FString(TEXT("SavedInputData/TestData.json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;

	FString inputData;
	bool success = FFileHelper::LoadFileToString(inputData, *AbsoluteFilePath);
	if (success) {
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(inputData);
		success = FJsonSerializer::Deserialize(JsonReader, JsonParsed);
		if (success) {
			auto JsonArray = JsonParsed->GetArrayField("inputStates");
			for (const auto& v : JsonArray) {
				auto JsonStringValue = v.Get()->AsString();

				TSharedPtr<FJsonObject> JsonObjParsed;
				TSharedRef<TJsonReader<TCHAR>> JsonObjReader = TJsonReaderFactory<TCHAR>::Create(JsonStringValue);
				success = FJsonSerializer::Deserialize(JsonObjReader, JsonObjParsed);
				if (!success) {
					UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON input object"));
				}
		
				InputState state;
				state.speed = JsonObjParsed->GetNumberField("speed");
				state.horizontal = JsonObjParsed->GetNumberField("horizontal");
				state.vertical = JsonObjParsed->GetNumberField("vertical");
				inputSequence.Add(state);
			}
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON input array object"));
	}
}
