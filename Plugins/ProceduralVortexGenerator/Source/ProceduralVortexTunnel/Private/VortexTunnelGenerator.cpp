// Copyright 2018-2025 Athian Games. All Rights Reserved. 


#include "VortexTunnelGenerator.h"
#include "VortexSpline.h"
#include "FVortexMeshGenerationTask.h"
#include "KismetProceduralMeshLibrary.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"

AVortexTunnelGenerator::AVortexTunnelGenerator()
{
    PrimaryActorTick.bCanEverTick = true;
    
    RootBillboard = CreateDefaultSubobject<UBillboardComponent>("RootBillboard");
    RootComponent = RootBillboard;

    MeshGenerator = CreateDefaultSubobject<UVortexMeshGenerator>("MeshGenerator");

    PreviewMesh = CreateDefaultSubobject<UProceduralMeshComponent>("PreviewMesh");
    PreviewMesh->SetupAttachment(RootComponent);

    NumberOfSegments = 10;
    RelativeStartDistance = 0.0f;
    RelativeEndDistance = 100.0f;
}

void AVortexTunnelGenerator::BeginPlay()
{
    Super::BeginPlay();
}

void AVortexTunnelGenerator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (CurrentTask && CurrentTask->IsDone())
    {
        ProcessNextTask();
    }
}

void AVortexTunnelGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    if (bIsPreviewMesh)
    {
        GeneratePreviewVortex();
    }

    if (VortexMaterial)
    {
        PreviewMesh->SetMaterial(0, VortexMaterial);
    }
    
}

void AVortexTunnelGenerator::GeneratePreviewVortex()
{
    bIsPreviewMesh = true;
    if (PreviewMesh)
    {
       // PreviewMesh->ClearAllMeshSections();
        
        // Create a new task
        auto NewTask = MakeUnique<FAsyncTask<FVortexMeshGenerationTask>>(this, MeshGenerator, VortexProperties);

        // If there's no current task, start this one immediately
        if (!CurrentTask)
        {
            CurrentTask = MoveTemp(NewTask);
            CurrentTask->StartBackgroundTask();
        }
        else
        {
            // Otherwise, replace any existing queued task with this new one
            PendingTask = MoveTemp(NewTask);
        }
    }
}


bool AVortexTunnelGenerator::ShouldTickIfViewportsOnly() const
{
    return true;
}

void AVortexTunnelGenerator::GenerateVortexTunnel()
{
    if (!SourceSpline || !SourceSpline->SplineComponent || !PreviewMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Spline or Preview Mesh"));
        return;
    }

    PreviewMesh->ClearAllMeshSections();

    USplineComponent* SplineComponent = SourceSpline->SplineComponent;
    float SplineLength = SplineComponent->GetSplineLength();
    float StartDistance = FMath::Clamp(RelativeStartDistance * SplineLength / 100.0f, 0.0f, SplineLength);
    float EndDistance = FMath::Clamp(RelativeEndDistance * SplineLength / 100.0f, StartDistance, SplineLength);

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector2D> UV0;

    int32 NumRings = FMath::Max(VortexProperties.HeightSegments, 2);
    int32 NumSides = FMath::Max(VortexProperties.RadialSegments, 3);
    float Radius = FMath::Max(VortexProperties.Radius, 1.0f);

    for (int32 Ring = 0; Ring <= NumRings; ++Ring)
    {
        float Distance = FMath::Lerp(StartDistance, EndDistance, static_cast<float>(Ring) / NumRings);
        
        FVector SplineLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        FVector SplineTangent = SplineComponent->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();
        FVector SplineUp = SplineComponent->GetUpVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        FVector SplineRight = FVector::CrossProduct(SplineTangent, SplineUp).GetSafeNormal();
        
        // Recalculate SplineUp to ensure orthogonality
        SplineUp = FVector::CrossProduct(SplineRight, SplineTangent).GetSafeNormal();

        for (int32 Side = 0; Side <= NumSides; ++Side)
        {
            float Angle = (static_cast<float>(Side) / NumSides) * 2.0f * PI;
            
            FVector Offset = SplineRight * FMath::Cos(Angle) * Radius + SplineUp * FMath::Sin(Angle) * Radius;
            FVector VertexPosition = SplineLocation + Offset;

            Vertices.Add(VertexPosition);
            UV0.Add(FVector2D(static_cast<float>(Side) / NumSides, static_cast<float>(Ring) / NumRings));

            if (Ring < NumRings && Side < NumSides)
            {
                int32 CurrentVertex = Ring * (NumSides + 1) + Side;
                int32 NextVertex = CurrentVertex + 1;
                int32 VertexBelow = CurrentVertex + (NumSides + 1);
                int32 VertexBelowNext = VertexBelow + 1;

                Triangles.Add(CurrentVertex);
                Triangles.Add(VertexBelow);
                Triangles.Add(NextVertex);

                Triangles.Add(NextVertex);
                Triangles.Add(VertexBelow);
                Triangles.Add(VertexBelowNext);
            }
        }
    }

    TArray<FVector> Normals;
    TArray<FProcMeshTangent> Tangents;
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);

    PreviewMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, TArray<FLinearColor>(), Tangents, true);
    
    if (VortexMaterial)
    {
        PreviewMesh->SetMaterial(0, VortexMaterial);
    }
}

void AVortexTunnelGenerator::CleanupMesh(TArray<FVector>& Vertices, TArray<int32>& Triangles)
{
    if (Vertices.Num() == 0 || Triangles.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CleanupMesh: Empty input data"));
        return;
    }

    // Remove duplicate vertices
    TMap<FVector, int32> UniqueVertices;
    TArray<int32> OldToNewVertexMap;
    TArray<FVector> NewVertices;
    NewVertices.Reserve(Vertices.Num()); // Pre-allocate memory

    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        if (!Vertices[i].IsNearlyZero()) // Check for invalid vertices
        {
            const int32* ExistingIndex = UniqueVertices.Find(Vertices[i]);
            if (ExistingIndex)
            {
                OldToNewVertexMap.Add(*ExistingIndex);
            }
            else
            {
                int32 NewIndex = NewVertices.Add(Vertices[i]);
                UniqueVertices.Add(Vertices[i], NewIndex);
                OldToNewVertexMap.Add(NewIndex);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CleanupMesh: Invalid vertex detected at index %d"), i);
            OldToNewVertexMap.Add(INDEX_NONE);
        }
    }

    // Create a new array for updated triangles
    TArray<int32> NewTriangles;
    NewTriangles.Reserve(Triangles.Num());

    // Update triangle indices and remove degenerate triangles
    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        if (i + 2 < Triangles.Num()) // Ensure we have a full triangle
        {
            int32 A = Triangles[i] < OldToNewVertexMap.Num() ? OldToNewVertexMap[Triangles[i]] : INDEX_NONE;
            int32 B = Triangles[i+1] < OldToNewVertexMap.Num() ? OldToNewVertexMap[Triangles[i+1]] : INDEX_NONE;
            int32 C = Triangles[i+2] < OldToNewVertexMap.Num() ? OldToNewVertexMap[Triangles[i+2]] : INDEX_NONE;

            if (A != INDEX_NONE && B != INDEX_NONE && C != INDEX_NONE && A != B && B != C && C != A)
            {
                NewTriangles.Add(A);
                NewTriangles.Add(B);
                NewTriangles.Add(C);
            }
        }
    }

    // Replace old arrays with new ones
    Vertices = MoveTemp(NewVertices);
    Triangles = MoveTemp(NewTriangles);

    UE_LOG(LogTemp, Log, TEXT("CleanupMesh: Vertices reduced from %d to %d, Triangles from %d to %d"),
           OldToNewVertexMap.Num(), Vertices.Num(), Triangles.Num() / 3, NewTriangles.Num() / 3);
}

void AVortexTunnelGenerator::ReceiveGeneratedMeshData(TArray<FVector>& Vertices, TArray<int32>& Triangles,
                                                      TArray<FVector>& Normals, TArray<FVector2D>& UV0,
                                                      TArray<FProcMeshTangent>& Tangents)
{
    if (!PreviewMesh || Vertices.Num() == 0 || Triangles.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReceiveGeneratedMeshData: Invalid input data"));
        return;
    }

    // Validate input data
    if (Vertices.Num() != Normals.Num() || Vertices.Num() != UV0.Num() || Vertices.Num() != Tangents.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("ReceiveGeneratedMeshData: Mismatched array sizes in mesh data"));
        return;
    }

    CleanupMesh(Vertices, Triangles);

    // Check if we still have valid data after cleanup
    if (Vertices.Num() == 0 || Triangles.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReceiveGeneratedMeshData: No valid mesh data after cleanup"));
        return;
    }

    FProcMeshSection ProcMeshSection;

    // Convert vertex data to FProcMeshVertex
    TArray<FProcMeshVertex> ProcVertices;
    ProcVertices.SetNum(Vertices.Num());
    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        ProcVertices[i].Position = Vertices[i];
        ProcVertices[i].Normal = i < Normals.Num() ? Normals[i] : FVector::UpVector;
        ProcVertices[i].UV0 = i < UV0.Num() ? UV0[i] : FVector2D::ZeroVector;
        ProcVertices[i].Tangent = i < Tangents.Num() ? FProcMeshTangent(Tangents[i].TangentX, false) : FProcMeshTangent(1, 0, 0);
        ProcVertices[i].Color = FColor::White;
    }

    // Assign vertex buffer
    ProcMeshSection.ProcVertexBuffer = ProcVertices;

    // Convert and assign index buffer
    TArray<uint32> ProcIndices;
    ProcIndices.SetNum(Triangles.Num());
    for (int32 i = 0; i < Triangles.Num(); ++i)
    {
        if (Triangles[i] < 0 || Triangles[i] >= Vertices.Num())
        {
            UE_LOG(LogTemp, Warning, TEXT("ReceiveGeneratedMeshData: Invalid triangle index detected"));
            return;
        }
        ProcIndices[i] = static_cast<uint32>(Triangles[i]);
    }
    ProcMeshSection.ProcIndexBuffer = ProcIndices;

    // Set other properties
    ProcMeshSection.SectionLocalBox = FBox(Vertices);
    ProcMeshSection.bEnableCollision = false; // Temporarily disable collision

    // Use SetProcMeshSection with the created FProcMeshSection
    PreviewMesh->SetProcMeshSection(0, ProcMeshSection);
}

void AVortexTunnelGenerator::ProcessNextTask()
{
    if (CurrentTask && CurrentTask->IsDone())
    {
        FVortexMeshGenerationTask* Task = &CurrentTask->GetTask();
        ReceiveGeneratedMeshData(Task->GetVertices(), Task->GetTriangles(), Task->GetNormals(), Task->GetUV0(), Task->GetTangents());

        CurrentTask.Reset();

        // If there's a pending task, start it now
        if (PendingTask)
        {
            CurrentTask = MoveTemp(PendingTask);
            CurrentTask->StartBackgroundTask();
        }
    }
}

