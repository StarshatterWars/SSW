// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HAL/Platform.h"

struct FMeshDescription;

class UWorldScapeMeshComponent;

FMeshDescription
WORLDSCAPECORE_API
BuildMeshDescription(UWorldScapeMeshComponent* ProcMeshComp );

void
WORLDSCAPECORE_API
MeshDescriptionToProcMesh( const FMeshDescription& MeshDescription, UWorldScapeMeshComponent* ProcMeshComp );


