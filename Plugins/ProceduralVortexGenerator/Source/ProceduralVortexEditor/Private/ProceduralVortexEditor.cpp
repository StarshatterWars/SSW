// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#include "ProceduralVortexEditor.h"

#include "AssetToolsModule.h"
#include "FVortexAssetTypeAction.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "FProceduralVortexEditorModule"

void FProceduralVortexEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	ProceduralAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Procedural Vortex")), LOCTEXT("Procedural Vortex", "Procedural Vortex"));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FVortexDataTypeAction(ProceduralAssetCategoryBit)));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FVortexFXDataTypeAction(ProceduralAssetCategoryBit)));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FVortexRepeaterDataTypeAction(ProceduralAssetCategoryBit)));
}

void FProceduralVortexEditorModule::ShutdownModule()
{

}

void FProceduralVortexEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools,
	TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProceduralVortexEditorModule, ProceduralVortexEditor)