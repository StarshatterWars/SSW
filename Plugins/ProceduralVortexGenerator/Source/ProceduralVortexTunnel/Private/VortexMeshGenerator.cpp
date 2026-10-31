// Copyright 2018-2025 Athian Games. All Rights Reserved. 


#include "VortexMeshGenerator.h"

#include "VortexTunnelGenerator.h"
#include "ProceduralMeshComponent/Public/KismetProceduralMeshLibrary.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Async/ParallelFor.h"

#define LOCTEXT_NAMESPACE "VortexMeshGenerator"

UVortexMeshGenerator::UVortexMeshGenerator()
{
}

void UVortexMeshGenerator::GenerateVortexMesh(UProceduralMeshComponent* Mesh, const FVortexParameters& Params)
{
    if(Mesh)
    {
        Mesh->ClearAllMeshSections();
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UV0;
        TArray<FProcMeshTangent> Tangents;
        GenerateVortexVertices(Vertices, Triangles, UV0, Params);
    
        // Recalculate normals
        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);
    
        // Ensure normals are pointing outwards
        for (int32 i = 0; i < Normals.Num(); ++i)
        {
            Normals[i] = (Vertices[i] - FVector(0, 0, Vertices[i].Z)).GetSafeNormal();
        }
    
        Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, TArray<FLinearColor>(), Tangents, true);
    }
}

float UVortexMeshGenerator::SimpleNoise3D(FVector v)
{
    // Simple hash function
    auto hash = [](float n) -> float {
        float x = FMath::Sin(n) * 43758.5453f;
        return x - FMath::Floor(x);
    };

    FVector floor = FVector(FMath::Floor(v.X), FMath::Floor(v.Y), FMath::Floor(v.Z));
    FVector frac = v - floor;

    float n = FVector::DotProduct(floor, FVector(37.0f, 57.0f, 113.0f));
    float res = FMath::Lerp(
        FMath::Lerp(
            FMath::Lerp(hash(n + 0.0f), hash(n + 37.0f), frac.X),
            FMath::Lerp(hash(n + 57.0f), hash(n + 94.0f), frac.X),
            frac.Y),
        FMath::Lerp(
            FMath::Lerp(hash(n + 113.0f), hash(n + 150.0f), frac.X),
            FMath::Lerp(hash(n + 170.0f), hash(n + 207.0f), frac.X),
            frac.Y),
        frac.Z);

    return res * 2.0f - 1.0f; // Map to [-1, 1] range
}

void UVortexMeshGenerator::GenerateVortexVertices(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params)
{
    const int32 MaxVertices = 1000000;
    const int32 MaxTriangles = 2000000;

    switch (Params.VortexType)
    {
    case EVortexType::Normal:
        GenerateNormalVortex(Vertices, Triangles, UV0, Params);
        break;
    case EVortexType::Wired:
        GenerateWiredVortex(Vertices, Triangles, UV0, Params);
        break;
    case EVortexType::CubeCluster:
        GenerateCubeClusterVortex(Vertices, Triangles, UV0, Params);
        break;
    case EVortexType::Ring:
        GenerateRingVortex(Vertices, Triangles, UV0, Params);
        break;
    default:
        GenerateNormalVortex(Vertices, Triangles, UV0, Params);
        break;
    }

    if (Vertices.Num() > MaxVertices)
    {
        Vertices.SetNum(MaxVertices);
        UV0.SetNum(MaxVertices);
    }
    if (Triangles.Num() > MaxTriangles)
    {
        Triangles.SetNum(MaxTriangles);
    }
}

FVector UVortexMeshGenerator::GenerateOrganicNoise(const FVector& Position, const FVortexParameters& Params)
{
    // Simple Perlin noise implementation
    FVector NoiseInput = Position * 0.01f;
    return FVector(
        FMath::PerlinNoise3D(NoiseInput),
        FMath::PerlinNoise3D(NoiseInput + FVector(31.416f, 27.183f, 23.14f)),
        FMath::PerlinNoise3D(NoiseInput + FVector(-12.345f, 67.890f, -45.678f))
    );
}

void UVortexMeshGenerator::GenerateTriangles(TArray<int32>& Triangles, int32 RadialSegments, int32 HeightSegments)
{
    Triangles.Empty();
    Triangles.Reserve(RadialSegments * HeightSegments * 4 * 3);

    for (int32 y = 0; y < HeightSegments; ++y)
    {
        for (int32 x = 0; x < RadialSegments; ++x)
        {
            int32 current = (y * (RadialSegments + 1) + x) * 2;
            int32 next = current + 2;
            int32 below = ((y + 1) * (RadialSegments + 1) + x) * 2;
            int32 belowNext = below + 2;

            // Outer surface
            AddTriangle(Triangles, current, below, next);
            AddTriangle(Triangles, next, below, belowNext);

            // Inner surface
            AddTriangle(Triangles, current + 1, next + 1, below + 1);
            AddTriangle(Triangles, next + 1, belowNext + 1, below + 1);

            // Connect outer to inner
            AddTriangle(Triangles, current, next, current + 1);
            AddTriangle(Triangles, next, next + 1, current + 1);
        }
    }
}

void UVortexMeshGenerator::AddTriangle(TArray<int32>& Triangles, int32 V1, int32 V2, int32 V3)
{
    Triangles.Add(V1);
    Triangles.Add(V2);
    Triangles.Add(V3);

    Triangles.Add(V3);
    Triangles.Add(V2);
    Triangles.Add(V1);
}

float UVortexMeshGenerator::FractalNoise(FVector v, int32 octaves, float persistence, float lacunarity)
{
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int32 i = 0; i < octaves; ++i)
    {
        total += FMath::PerlinNoise3D(v * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / maxValue;
}

void UVortexMeshGenerator::GenerateNormalVortex(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params)
{
    const int32 RadialSegments = FMath::Max(Params.RadialSegments, 3);
    const int32 HeightSegments = FMath::Max(Params.HeightSegments, 1);

    for (int32 Y = 0; Y <= HeightSegments; Y++)
    {
        float V = static_cast<float>(Y) / HeightSegments;
        float Height = V * Params.Height - (Params.Height * 0.5f);
        float Radius = FMath::Lerp(Params.Radius * Params.TaperBottom, Params.Radius * Params.TaperTop, V);
        float Twist = V * Params.TwistAmount;

        for (int32 X = 0; X <= RadialSegments; X++)
        {
            float U = static_cast<float>(X) / RadialSegments;
            float Angle = U * 2.0f * PI + Twist;
            
            FVector Position(
                Radius * FMath::Cos(Angle),
                Radius * FMath::Sin(Angle),
                Height
            );

            // Apply wave effect
            Position += FVector(0, 0, FMath::Sin(V * Params.WaveFrequency * PI * 2.0f + Params.WavePhase) * Params.WaveAmplitude);

            // Apply organic distortion
            Position += GenerateOrganicNoise(Position, Params) * Params.OrganicIntensity;

            Vertices.Add(Position);
            UV0.Add(FVector2D(U, V));

            if (Y < HeightSegments && X < RadialSegments)
            {
                int32 Current = Y * (RadialSegments + 1) + X;
                int32 Next = Current + 1;
                int32 Below = (Y + 1) * (RadialSegments + 1) + X;
                int32 BelowNext = Below + 1;

                Triangles.Add(Current);
                Triangles.Add(Below);
                Triangles.Add(Next);

                Triangles.Add(Next);
                Triangles.Add(Below);
                Triangles.Add(BelowNext);
            }
        }
    }
}

void UVortexMeshGenerator::GenerateWiredVortex(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params)
{
    const int32 RadialSegments = FMath::Max(Params.RadialSegments, 8);
    const int32 HeightSegments = FMath::Max(Params.HeightSegments, 8);

    // Generate vertical wires
    for (int32 X = 0; X < RadialSegments; X++)
    {
        float U = static_cast<float>(X) / RadialSegments;
        float BaseAngle = U * 2.0f * PI;

        for (int32 Y = 0; Y <= HeightSegments; Y++)
        {
            float V = static_cast<float>(Y) / HeightSegments;
            float Height = V * Params.Height - (Params.Height * 0.5f);
            float Radius = FMath::Lerp(Params.Radius * Params.TaperBottom, Params.Radius * Params.TaperTop, V);
            float Twist = V * Params.TwistAmount + Params.TwistOffset;
            float Angle = BaseAngle + Twist;

            // Apply wave effect
            Height += FMath::Sin(V * Params.WaveFrequency * PI * 2.0f + Params.WavePhase) * Params.WaveAmplitude;

            FVector WireCenter(
                Radius * FMath::Cos(Angle),
                Radius * FMath::Sin(Angle),
                Height
            );

            // Generate vertices around the wire
            for (int32 W = 0; W < Params.WireSegments; W++)
            {
                float WireAngle = (float)W / Params.WireSegments * 2.0f * PI;
                FVector WireOffset = (FVector::RightVector * FMath::Cos(WireAngle) + FVector::ForwardVector * FMath::Sin(WireAngle)) * Params.WireThickness;
                FVector RotatedOffset = WireOffset.RotateAngleAxis(Angle * 180.0f / PI, FVector::UpVector);
                FVector VertexPosition = WireCenter + RotatedOffset;

                // Apply organic distortion
                VertexPosition += GenerateOrganicNoise(VertexPosition, Params) * Params.OrganicIntensity;

                Vertices.Add(VertexPosition);
                UV0.Add(FVector2D(U, V));

                // Generate triangles
                if (Y < HeightSegments && W < Params.WireSegments)
                {
                    int32 CurrentIndex = Vertices.Num() - 1;
                    int32 NextIndex = CurrentIndex + 1;
                    int32 BelowIndex = CurrentIndex + Params.WireSegments;
                    int32 BelowNextIndex = BelowIndex + 1;

                    if (W == Params.WireSegments - 1)
                    {
                        NextIndex -= Params.WireSegments;
                        BelowNextIndex -= Params.WireSegments;
                    }

                    Triangles.Add(CurrentIndex);
                    Triangles.Add(BelowIndex);
                    Triangles.Add(NextIndex);

                    Triangles.Add(NextIndex);
                    Triangles.Add(BelowIndex);
                    Triangles.Add(BelowNextIndex);
                }
            }
        }
    }

    // Generate horizontal rings
    for (int32 Y = 0; Y <= HeightSegments; Y += Params.RingInterval)
    {
        float V = static_cast<float>(Y) / HeightSegments;
        float Height = V * Params.Height - (Params.Height * 0.5f);
        float Radius = FMath::Lerp(Params.Radius * Params.TaperBottom, Params.Radius * Params.TaperTop, V);
        float Twist = V * Params.TwistAmount + Params.TwistOffset;

        // Apply wave effect
        Height += FMath::Sin(V * Params.WaveFrequency * PI * 2.0f + Params.WavePhase) * Params.WaveAmplitude;

        int32 RingBaseIndex = Vertices.Num();

        for (int32 X = 0; X <= RadialSegments; X++)
        {
            float U = static_cast<float>(X) / RadialSegments;
            float Angle = U * 2.0f * PI + Twist;

            FVector RingCenter(
                Radius * FMath::Cos(Angle),
                Radius * FMath::Sin(Angle),
                Height
            );

            for (int32 W = 0; W < Params.WireSegments; W++)
            {
                float WireAngle = (float)W / Params.WireSegments * 2.0f * PI;
                FVector WireOffset = (FVector::UpVector * FMath::Cos(WireAngle) + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * FMath::Sin(WireAngle)) * Params.WireThickness;
                FVector VertexPosition = RingCenter + WireOffset;

                // Apply organic distortion
                VertexPosition += GenerateOrganicNoise(VertexPosition, Params) * Params.OrganicIntensity;

                Vertices.Add(VertexPosition);
                UV0.Add(FVector2D(U, V));

                // Generate triangles for rings
                if (X < RadialSegments && W < Params.WireSegments)
                {
                    int32 CurrentIndex = Vertices.Num() - 1;
                    int32 NextIndex = CurrentIndex + 1;
                    int32 BelowIndex = CurrentIndex + Params.WireSegments;
                    int32 BelowNextIndex = BelowIndex + 1;

                    if (W == Params.WireSegments - 1)
                    {
                        NextIndex -= Params.WireSegments;
                        BelowNextIndex -= Params.WireSegments;
                    }

                    Triangles.Add(CurrentIndex);
                    Triangles.Add(BelowIndex);
                    Triangles.Add(NextIndex);

                    Triangles.Add(NextIndex);
                    Triangles.Add(BelowIndex);
                    Triangles.Add(BelowNextIndex);
                }
            }
        }
    }
}

void UVortexMeshGenerator::GenerateCubeClusterVortex(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params)
{
    const int32 RadialSegments = FMath::Max(Params.RadialSegments, 8);
    const int32 HeightSegments = FMath::Max(Params.HeightSegments, 4);
    const int32 TotalCubes = RadialSegments * HeightSegments;

    Vertices.Reset(TotalCubes * 8);
    Triangles.Reset(TotalCubes * 36);
    UV0.Reset(TotalCubes * 8);

    for (int32 Y = 0; Y < HeightSegments; Y++)
    {
        float V = static_cast<float>(Y) / HeightSegments;
        float Height = V * Params.Height - (Params.Height * 0.5f);
        float Radius = FMath::Lerp(Params.Radius * Params.TaperBottom, Params.Radius * Params.TaperTop, V);
        float Twist = V * Params.TwistAmount;

        for (int32 X = 0; X < RadialSegments; X++)
        {
            float U = static_cast<float>(X) / RadialSegments;
            float Angle = U * 2.0f * PI + Twist;

            FVector SurfacePoint(
                Radius * FMath::Cos(Angle),
                Radius * FMath::Sin(Angle),
                Height + FMath::Sin(V * Params.WaveFrequency * PI * 2.0f + Params.WavePhase) * Params.WaveAmplitude
            );

            // Calculate surface normal (pointing outwards)
            FVector SurfaceNormal = SurfacePoint.GetSafeNormal();

            // Calculate cube position (moved inwards by half the cube size)
            float CubeSize = FMath::Lerp(Params.MinCubeSize, Params.MaxCubeSize, FMath::FRand());
            FVector CubePosition = SurfacePoint - SurfaceNormal * (CubeSize * 0.5f);

            // Create rotation matrix with X-axis aligned to -SurfaceNormal (facing inwards)
            FVector UpVector = FVector::UpVector;
            FVector RightVector = FVector::CrossProduct(UpVector, -SurfaceNormal).GetSafeNormal();
            UpVector = FVector::CrossProduct(-SurfaceNormal, RightVector);
            FMatrix CubeRotationMatrix = FMatrix(RightVector, UpVector, -SurfaceNormal, FVector::ZeroVector);

            // Apply organic distortion to the cube position
            CubePosition += GenerateOrganicNoise(CubePosition, Params) * Params.OrganicIntensity;

            GenerateCube(Vertices, Triangles, UV0, CubePosition, CubeSize, CubeRotationMatrix.Rotator());
        }

        QueueProgressUpdate((float)(Y + 1) / HeightSegments, LOCTEXT("CubeClusterProgress", "Generating Cube Cluster Vortex"));
    }

    AsyncTask(ENamedThreads::GameThread, [this]() {
        ProcessQueuedProgressUpdates();
        UpdateProgress(1.0f, LOCTEXT("CubeClusterComplete", "Cube Cluster Vortex Generation Complete"));
        });
}

void UVortexMeshGenerator::GenerateCube(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVector& Center, float Size, const FRotator& Rotation)
{
    FVector HalfSize = FVector(Size * 0.5f);

    TArray<FVector> CubeVertices = {
        FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z),
        FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z),
        FVector(HalfSize.X,  HalfSize.Y, -HalfSize.Z),
        FVector(-HalfSize.X,  HalfSize.Y, -HalfSize.Z),
        FVector(-HalfSize.X, -HalfSize.Y,  HalfSize.Z),
        FVector(HalfSize.X, -HalfSize.Y,  HalfSize.Z),
        FVector(HalfSize.X,  HalfSize.Y,  HalfSize.Z),
        FVector(-HalfSize.X,  HalfSize.Y,  HalfSize.Z)
    };

    int32 BaseIndex = Vertices.Num();
    for (FVector& Vertex : CubeVertices)
    {
        Vertex = Rotation.RotateVector(Vertex) + Center;
        Vertices.Add(Vertex);
        UV0.Add(FVector2D::ZeroVector); // You may want to assign proper UVs here
    }

    // Define cube faces
    TArray<int32> CubeFaces = {
        0, 1, 2, 0, 2, 3, // Front
        1, 5, 6, 1, 6, 2, // Right
        5, 4, 7, 5, 7, 6, // Back
        4, 0, 3, 4, 3, 7, // Left
        3, 2, 6, 3, 6, 7, // Top
        4, 5, 1, 4, 1, 0  // Bottom
    };

    for (int32 Index : CubeFaces)
    {
        Triangles.Add(BaseIndex + Index);
    }
}

void UVortexMeshGenerator::GenerateRingVortex(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params)
{
    const int32 RadialSegments = FMath::Max(Params.RadialSegments, 16);
    const int32 HeightSegments = FMath::Max(Params.HeightSegments, 4);
    const int32 RingSegments = 8;

    Vertices.Reset((RadialSegments + 1) * (HeightSegments + 1) * (RingSegments + 1));
    UV0.Reset((RadialSegments + 1) * (HeightSegments + 1) * (RingSegments + 1));
    Triangles.Reset(RadialSegments * HeightSegments * RingSegments * 6);

    // Use atomic counter for thread-safe progress tracking
    FThreadSafeCounter ProcessedSegments(0);

    ParallelFor(HeightSegments + 1, [&](int32 Y)
    {
        float V = static_cast<float>(Y) / HeightSegments;
        float Height = V * Params.Height - (Params.Height * 0.5f);
        float Radius = FMath::Lerp(Params.Radius * Params.TaperBottom, Params.Radius * Params.TaperTop, V);
        float Twist = V * Params.TwistAmount;
        Height += FMath::Sin(V * Params.WaveFrequency * PI * 2.0f + Params.WavePhase) * Params.WaveAmplitude;
        float RingThickness = FMath::Lerp(Params.MinRingThickness, Params.MaxRingThickness, FMath::Frac(V * 5.0f));

        TArray<FVector> LocalVertices;
        TArray<int32> LocalTriangles;
        TArray<FVector2D> LocalUV0;

        for (int32 X = 0; X <= RadialSegments; X++)
        {
            float U = static_cast<float>(X) / RadialSegments;
            float Angle = U * 2.0f * PI + Twist;

            for (int32 R = 0; R <= RingSegments; R++)
            {
                float RingAngle = (float)R / RingSegments * 2.0f * PI;
                FVector RingOffset = (FVector::UpVector * FMath::Cos(RingAngle) + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * FMath::Sin(RingAngle)) * RingThickness;
                FVector VertexPosition(
                    (Radius + RingOffset.X) * FMath::Cos(Angle),
                    (Radius + RingOffset.X) * FMath::Sin(Angle),
                    Height + RingOffset.Z
                );

                VertexPosition += GenerateOrganicNoise(VertexPosition, Params) * Params.OrganicIntensity;

                LocalVertices.Add(VertexPosition);
                LocalUV0.Add(FVector2D(U, V));

                if (X < RadialSegments && R < RingSegments)
                {
                    int32 Current = X * (RingSegments + 1) + R;
                    int32 Next = Current + 1;
                    int32 Below = (X + 1) * (RingSegments + 1) + R;
                    int32 BelowNext = Below + 1;

                    LocalTriangles.Add(Current);
                    LocalTriangles.Add(Below);
                    LocalTriangles.Add(Next);

                    LocalTriangles.Add(Next);
                    LocalTriangles.Add(Below);
                    LocalTriangles.Add(BelowNext);
                }
            }
        }

        // Thread-safe addition of local data to main arrays
        {
            FScopeLock Lock(&ProgressLock);
            int32 BaseIndex = Vertices.Num();
            Vertices.Append(LocalVertices);
            UV0.Append(LocalUV0);
            for (int32 TriIndex : LocalTriangles)
            {
                Triangles.Add(BaseIndex + TriIndex);
            }
        }

        // Increment processed segments and update progress
        int32 Processed = ProcessedSegments.Increment();
        float Progress = static_cast<float>(Processed) / (HeightSegments + 1);
        AsyncTask(ENamedThreads::GameThread, [this, Progress]() {
            UpdateProgress(Progress, LOCTEXT("RingVortexProgress", "Generating Ring Vortex"));
        });
    });

    // Ensure final progress update is called
    AsyncTask(ENamedThreads::GameThread, [this]() {
        UpdateProgress(1.0f, LOCTEXT("RingVortexComplete", "Ring Vortex Generation Complete"));
    });
}


void UVortexMeshGenerator::QueueProgressUpdate(float Progress, const FText& StatusText)
{
    FScopeLock Lock(&ProgressLock);
    CurrentProgress = Progress;
    CurrentStatusText = StatusText;
}

void UVortexMeshGenerator::ProcessQueuedProgressUpdates()
{
    float Progress;
    FText StatusText;

    {
        FScopeLock Lock(&ProgressLock);
        Progress = CurrentProgress;
        StatusText = CurrentStatusText;
    }

    UpdateProgress(Progress, StatusText);
}

void UVortexMeshGenerator::UpdateProgress(float Progress, const FText& StatusText)
{
    if (!IsInGameThread())
    {
        AsyncTask(ENamedThreads::GameThread, [this, Progress, StatusText]() {
            UpdateProgress(Progress, StatusText);
        });
        return;
    }

    if (!ProgressNotification.IsValid())
    {
        FNotificationInfo Info(StatusText);
        Info.bFireAndForget = false;
        Info.FadeOutDuration = 0.5f;
        Info.ExpireDuration = 0.0f;
        Info.bUseThrobber = true;
        Info.bUseSuccessFailIcons = false;
        Info.bUseLargeFont = false;
        Info.bAllowThrottleWhenFrameRateIsLow = false;

        ProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);
        if (ProgressNotification.IsValid())
        {
            ProgressNotification->SetCompletionState(SNotificationItem::CS_Pending);
        }
    }

    if (ProgressNotification.IsValid())
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("Status"), StatusText);
        Args.Add(TEXT("Progress"), FText::AsPercent(Progress));
        FText ProgressText = FText::Format(LOCTEXT("ProgressNotification", "{Status} ({Progress})"), Args);

        ProgressNotification->SetText(ProgressText);

        if (FMath::IsNearlyEqual(Progress, 1.0f, 0.001f))
        {
            ProgressNotification->SetCompletionState(SNotificationItem::CS_Success);
            ProgressNotification->ExpireAndFadeout();
            ProgressNotification.Reset();
        }
    }
}

#undef LOCTEXT_NAMESPACE