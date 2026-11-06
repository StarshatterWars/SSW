// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapePCGNodeBase.h"
#include "PCGContext.h"
#include "Data/PCGSpatialData.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "WorldScapePCG.h"
#include "Engine/World.h"
#include "PCGComponent.h"
#include "EngineUtils.h"
#include "PCGPin.h"


#define LOCTEXT_NAMESPACE "WorldScapePCGNodeBase"

UWorldScapePCGNodeBaseSettings::UWorldScapePCGNodeBaseSettings()
{
	// Set default values
}

TArray<FPCGPinProperties> UWorldScapePCGNodeBaseSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultInputLabel, EPCGDataType::Spatial);
	return PinProperties;
}

TArray<FPCGPinProperties> UWorldScapePCGNodeBaseSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Spatial);
	return PinProperties;
}

FPCGElementPtr UWorldScapePCGNodeBaseSettings::CreateElement() const
{
	return MakeShared<FWorldScapePCGNodeBaseElement>();
}

FPCGContext* FWorldScapePCGNodeBaseElement::Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)
{
	FPCGContext* Context = new FPCGContext();
	Context->InputData = InputData;
	Context->SourceComponent = SourceComponent;
	Context->Node = Node;
	return Context;
}

bool FWorldScapePCGNodeBaseElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FWorldScapePCGNodeBaseElement::Execute);

	check(Context);

	const UWorldScapePCGNodeBaseSettings* Settings = Context->GetInputSettings<UWorldScapePCGNodeBaseSettings>();
	check(Settings);

	// Execute WorldScape-specific logic
	return ExecuteWorldScapeLogic(Context, Settings);
}

bool FWorldScapePCGNodeBaseElement::ExecuteWorldScapeLogic(FPCGContext* Context, const UWorldScapePCGNodeBaseSettings* Settings) const
{
	// Base implementation - just pass through the input data
	// Derived classes should override this method

	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputs();
	for (const FPCGTaggedData& Input : Inputs)
	{
		FPCGTaggedData& Output = Context->OutputData.TaggedData.Add_GetRef(Input);
		Output.Pin = PCGPinConstants::DefaultOutputLabel;
	}

	UE_LOG(LogWorldScapePCG, Log, TEXT("WorldScape PCG Node Base executed"));
	return true;
}

AWorldScapeRoot* FWorldScapePCGNodeBaseElement::GetWorldScapeRoot(const UWorldScapePCGNodeBaseSettings* Settings) const
{
	if (Settings && Settings->WorldScapeRoot.IsValid())
	{
		return Settings->WorldScapeRoot.Get();
	}

	// Try to find a WorldScape root in the world if none is specified
	if (UWorld* World = GEngine->GetCurrentPlayWorld())
	{
		for (TActorIterator<AWorldScapeRoot> ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			AWorldScapeRoot* WorldScapeRoot = *ActorIterator;
			if (IsValid(WorldScapeRoot))
			{
				return WorldScapeRoot;
			}
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE