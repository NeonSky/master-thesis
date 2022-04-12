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
	framesToCollect = 600; // data_collection_settings.secondsToRecord * 60; // TODO: assuming 60 FPS, but then the fixed dt should be 0.01667 instead of 0.02
}


void UDataCollector::update(UpdatePayload update_payload) {
	if (frameNumber == framesToCollect) {
		UE_LOG(LogTemp, Warning, TEXT("Recording finished."));
		if (data_collection_settings.shouldCollectInputStates) {
			saveInputToFile();
		}
		if (data_collection_settings.shouldCollectBoatData) {
			saveBoatDataToFile();
		}
		data_collection_settings.shouldCollectBoatData = false;
		data_collection_settings.shouldCollecteWaveTextures = false;
		data_collection_settings.shouldCollectInputStates = false;
		
		// inputPawn->playBackInputSequence = false; // TODO: stop the input playback
	}
	frameNumber++;

	if (data_collection_settings.shouldCollectInputStates) {
		inputStates.Add(update_payload);
	}
	
	if (data_collection_settings.shouldCollecteWaveTextures) {
		TArray<float> h_rtt_r_channel_data;
		shaderModule->ComputeSerialization(eWave_h_rtt, serialization_rtt, h_rtt_r_channel_data);
		saveeWaveDataToFile(h_rtt_r_channel_data); // TODO save v texture
	}

	if (data_collection_settings.shouldCollectBoatData) {
		// Currently only supports ONE boat
		for (auto boat : boats) {
			if (boat) {
				FVector boatPos = boat->WorldPosition3D();
				boatPositions.Add(boat->WorldPosition3D());
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
	FString fname = *FString(TEXT("SavedInputData/") + data_collection_settings.inputFileName + TEXT(".json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
	TSharedRef<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> states;
	for (const auto& state : inputStates) {
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetNumberField("speed", state.speed_input);
		JsonObject->SetNumberField("horizontal", state.velocity_input.X);
		JsonObject->SetNumberField("vertical", state.velocity_input.Y);

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
static int ii = 0;
void UDataCollector::saveBoatDataToFile() {
	FString fname = *FString(TEXT("SavedBoatData/") + data_collection_settings.folderName + TEXT("/") + data_collection_settings.fileName + FString::FromInt(ii++) + TEXT(".json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
	TSharedRef<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> boatStates;
	for (const auto& pos : boatPositions) {
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetNumberField("position_x", pos.X);
		JsonObject->SetNumberField("position_y", pos.Y);
		JsonObject->SetNumberField("position_z", pos.Z);

		FString OutputString;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		boatStates.Add(MakeShareable(new FJsonValueString(OutputString)));
	}
	JsonRootObject->SetArrayField("boatPositions", boatStates);

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonRootObject, Writer);

	FFileHelper::SaveStringToFile(OutputString, *AbsoluteFilePath);
}

void UDataCollector::readInputJSON(TArray<UpdatePayload>& inputSequence) {
	FString fname = *FString(TEXT("SavedInputData/") + data_collection_settings.inputFileName + TEXT(".json"));
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
		
				UpdatePayload state;
				state.speed_input = JsonObjParsed->GetNumberField("speed");
				state.velocity_input.X = JsonObjParsed->GetNumberField("horizontal");
				state.velocity_input.Y = JsonObjParsed->GetNumberField("vertical");
				inputSequence.Add(state);
			}
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON input array object"));
		// inputPawn->playBackInputSequence = data_collection_settings.shouldPlayBackInputSequence = false; // TODO: fix
	}
}
