// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "VortexDataFactory.generated.h"

UCLASS()
class PROCEDURALVORTEXEDITOR_API UVortexFXDataFactory : public UFactory
{
	GENERATED_BODY()

public:

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface

	UVortexFXDataFactory();
};

UCLASS()
class PROCEDURALVORTEXEDITOR_API UVortexRepeaterDataFactory : public UFactory
{
	GENERATED_BODY()

public:

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface

	UVortexRepeaterDataFactory();
};
/**
 * 
 */
UCLASS()
class PROCEDURALVORTEXEDITOR_API UVortexDataFactory : public UFactory
{
	GENERATED_BODY()

public:

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface

	UVortexDataFactory();
};

