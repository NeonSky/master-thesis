#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SHADERMODELS_API ShaderModelsModule : public IModuleInterface {
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void Test(float f);
};
