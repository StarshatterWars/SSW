// Copyright 2018-2025 Athian Games. All Rights Reserved. 


#include "VortexDataFactory.h"

#include "VortexData.h"
#include "VortexFX.h"
#include "VortexRepeater.h"

bool UVortexFXDataFactory::ConfigureProperties()
{
	return Super::ConfigureProperties();
}

UVortexFXDataFactory::UVortexFXDataFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UVortexFXData::StaticClass();

}

UObject* UVortexFXDataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UVortexFXData* VortexFXData = nullptr;
	if (ensure(UVortexFXData::StaticClass() == Class))
	{
		ensure(0 != (RF_Public & Flags));
		VortexFXData = NewObject<UVortexFXData>(InParent, Name, Flags);

	}
	return VortexFXData;
}


UVortexRepeaterDataFactory::UVortexRepeaterDataFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UVortexRepeaterData::StaticClass();

}

bool UVortexRepeaterDataFactory::ConfigureProperties()
{
	return Super::ConfigureProperties();
}

UObject* UVortexRepeaterDataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UVortexRepeaterData* VortexRepeaterData = nullptr;
	if (ensure(UVortexRepeaterData::StaticClass() == Class))
	{
		ensure(0 != (RF_Public & Flags));
		VortexRepeaterData = NewObject<UVortexRepeaterData>(InParent, Name, Flags);

	}
	return VortexRepeaterData;
}

UVortexDataFactory::UVortexDataFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UVortexData::StaticClass();

}

bool UVortexDataFactory::ConfigureProperties()
{
	return true;
}

UObject* UVortexDataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UVortexData* VortexData = nullptr;
	if (ensure(UVortexData::StaticClass() == Class))
	{
		ensure(0 != (RF_Public & Flags));
		VortexData = NewObject<UVortexData>(InParent, Name, Flags);

	}
	return VortexData;
}
