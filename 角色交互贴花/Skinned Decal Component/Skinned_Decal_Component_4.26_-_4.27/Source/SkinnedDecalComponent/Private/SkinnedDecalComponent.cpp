// Copyright Eddie Ataberk 2021 All Rights Reserved.

#include "SkinnedDecalComponent.h"

#define LOCTEXT_NAMESPACE "FSkinnedDecalComponentModule"

void FSkinnedDecalComponentModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	//FString SkinnedDecalShaderDir = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("SkinnedDecalComponent/Source/SkinnedDecalComponent/Shader/"));
	//AddShaderSourceDirectoryMapping("/Plugin/SkinnedDecalComponent", SkinnedDecalShaderDir);

}

void FSkinnedDecalComponentModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSkinnedDecalComponentModule, SkinnedDecalComponent)