// Copyright 2018-2025 Athian Games. All Rights Reserved. 


#include "FVortexAssetTypeAction.h"
#include "VortexData.h"
#include "VortexRepeater.h"
#include "VortexFX.h"

#define LOCTEXT_NAMESPACE "VortexTunnelAssetTypeAction"

FVortexAssetTypeActionBase::FVortexAssetTypeActionBase(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
{

}


FColor FVortexAssetTypeActionBase::GetTypeColor() const
{
	return FColor::Cyan;
}



void FVortexAssetTypeActionBase::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);

}

uint32 FVortexAssetTypeActionBase::GetCategories()
{
	return MyAssetCategory;
}

void FVortexAssetTypeActionBase::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{


}


FText FVortexDataTypeAction::GetName() const
{
	return  NSLOCTEXT("VortexDataTypeAction", "VortexDataTypeAction", "Vortex Data");
}

UClass* FVortexDataTypeAction::GetSupportedClass() const
{
	return UVortexData::StaticClass();
}


FText FVortexFXDataTypeAction::GetName() const
{
	return  NSLOCTEXT("VortexFXDataTypeAction", "VortexFXDataTypeAction", "FX Data");
}

UClass* FVortexFXDataTypeAction::GetSupportedClass() const
{
	return UVortexFXData::StaticClass();
}


FText FVortexRepeaterDataTypeAction::GetName() const
{
	return  NSLOCTEXT("VortexRepeaterDataTypeAction", "VortexRepeaterDataTypeAction", "Repeater Data");
}

UClass* FVortexRepeaterDataTypeAction::GetSupportedClass() const
{
	return UVortexRepeaterData::StaticClass();
}

#undef LOCTEXT_NAMESPACE
