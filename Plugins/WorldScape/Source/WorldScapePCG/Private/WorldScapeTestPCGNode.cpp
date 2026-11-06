// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapeTestPCGNode.h"
#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Data/PCGSpatialData.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "WorldScapePCG.h"

#define LOCTEXT_NAMESPACE "WorldScapeTestPCGNode"

UWorldScapeTestPCGNodeSettings::UWorldScapeTestPCGNodeSettings()
{
	HeightOffset = 0.0f;
	bIncludeWater = false;
	HeightScale = 1.0f;
}

FPCGElementPtr UWorldScapeTestPCGNodeSettings::CreateElement() const
{
	return MakeShared<FWorldScapeTestPCGNodeElement>();
}

bool FWorldScapeTestPCGNodeElement::ExecuteWorldScapeLogic(FPCGContext* Context, const UWorldScapePCGNodeBaseSettings* BaseSettings) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FWorldScapeTestPCGNodeElement::Execute);

	const UWorldScapeTestPCGNodeSettings* Settings = CastChecked<UWorldScapeTestPCGNodeSettings>(BaseSettings);

	// Get WorldScape root
	AWorldScapeRoot* WorldScapeRoot = GetWorldScapeRoot(Settings);
	if (!WorldScapeRoot)
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("NoWorldScapeRoot", "No WorldScape Root found. Please assign one in the node settings or ensure one exists in the level."));
		return true;
	}

	// Process input data
	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputs();
	for (const FPCGTaggedData& Input : Inputs)
	{
		const UPCGSpatialData* SpatialData = Cast<UPCGSpatialData>(Input.Data);
		if (!SpatialData)
		{
			// If it's not spatial data, just pass it through
			FPCGTaggedData& Output = Context->OutputData.TaggedData.Add_GetRef(Input);
			Output.Pin = PCGPinConstants::DefaultOutputLabel;
			continue;
		}

		// Convert to point data if possible
		const UPCGPointData* PointData = SpatialData->ToPointData(Context);
		if (!PointData)
		{
			// If we can't convert to points, pass the original data through
			FPCGTaggedData& Output = Context->OutputData.TaggedData.Add_GetRef(Input);
			Output.Pin = PCGPinConstants::DefaultOutputLabel;
			continue;
		}

		// Create new point data
		UPCGPointData* OutputPointData = NewObject<UPCGPointData>();
		OutputPointData->InitializeFromData(PointData);

		TArray<FPCGPoint>& OutputPoints = OutputPointData->GetMutablePoints();
		const TArray<FPCGPoint>& InputPoints = PointData->GetPoints();

		OutputPoints.Reserve(InputPoints.Num());

		// Process each point
		for (const FPCGPoint& Point : InputPoints)
		{
			FPCGPoint NewPoint = Point;
			FVector Location = Point.Transform.GetLocation();
			const double H = WorldScapeRoot->GetGroundHeight(Location, Settings->bIncludeWater);
			const FVector Dir = (Location-WorldScapeRoot->GetActorLocation()).GetSafeNormal();
			NewPoint.Transform.SetLocation(WorldScapeRoot->GetActorLocation() + ( Dir*(WorldScapeRoot->PlanetScale+H+Settings->HeightOffset)));
			NewPoint.Transform.SetRotation(FRotationMatrix::MakeFromZ(Dir).ToQuat());
			OutputPoints.Add(NewPoint);
		}

		// Add output data
		OutputPointData->SetPoints(OutputPoints);
		
		FPCGTaggedData& Output = Context->OutputData.TaggedData.Emplace_GetRef();
		Output.Data = OutputPointData;
		Output.Pin = PCGPinConstants::DefaultOutputLabel;
		Output.Tags = Input.Tags;

		UE_LOG(LogWorldScapePCG, Log, TEXT("WorldScape Test PCG Node processed %d points"), OutputPoints.Num());
	}

	return true;
}

#undef LOCTEXT_NAMESPACE