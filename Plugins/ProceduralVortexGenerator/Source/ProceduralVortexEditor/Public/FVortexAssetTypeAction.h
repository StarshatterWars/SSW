// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

/**
 * 
 */
class PROCEDURALVORTEXEDITOR_API FVortexAssetTypeActionBase: public FAssetTypeActions_Base
{
public:
	explicit FVortexAssetTypeActionBase(EAssetTypeCategories::Type InAssetCategory);

	// IAssetTypeActions interface
	virtual FColor GetTypeColor() const override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return true; }
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects,
		TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;
	// End of IAssetTypeActions interface

	private:

	EAssetTypeCategories::Type MyAssetCategory;
};


class PROCEDURALVORTEXEDITOR_API FVortexDataTypeAction final : public FVortexAssetTypeActionBase
{
public:
	explicit FVortexDataTypeAction(EAssetTypeCategories::Type in_asset_category)
		: FVortexAssetTypeActionBase(in_asset_category)
	{
	}

	// IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;

};

class PROCEDURALVORTEXEDITOR_API FVortexFXDataTypeAction final : public FVortexAssetTypeActionBase
{
public:
	explicit FVortexFXDataTypeAction(EAssetTypeCategories::Type in_asset_category)
		: FVortexAssetTypeActionBase(in_asset_category)
	{
	}

	// IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;

};


class PROCEDURALVORTEXEDITOR_API FVortexRepeaterDataTypeAction final : public FVortexAssetTypeActionBase
{
public:
	explicit FVortexRepeaterDataTypeAction(EAssetTypeCategories::Type in_asset_category)
		: FVortexAssetTypeActionBase(in_asset_category)
	{
	}

	// IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;

};