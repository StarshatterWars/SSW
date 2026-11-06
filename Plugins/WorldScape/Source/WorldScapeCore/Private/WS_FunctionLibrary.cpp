

#include "WS_FunctionLibrary.h"

#include "Animation/AnimData/CurveIdentifier.h"
#include "Async/Async.h"
#include "Field/FieldSystemNoiseAlgo.h"
#include "Runtime/Core/Tests/Containers/TestUtils.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/ObjectWriter.h"


// ---------------------------------------------------
TArray<FVector> UWS_FunctionLibrary::WS_Sample_2DGrid( AWorldScapeRoot* Root , const FVector Center , const FRotator Rotation , const FVector Extend , const int32 Step )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_Sample_2DGrid);
	if (IsValid(Root))
	{
		TArray<FVector> Samples;
		const FVector Forward  = Rotation.Vector();
		const FVector RightVector = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
		const FVector Start = Forward*(Extend.X*-1) + RightVector*(Extend.Y*-1) + Center;
		const FVector LocalExtend = (Extend*2.0f) / Step;

		
		for (int32 x = 0; x < Step; x++)
		{
			for (int32 y = 0; y < Step; y++)
			{
				FVector SampleLocation = Start + ( ( ((x+0.5)*LocalExtend)*Forward) + (((y+0.5)*LocalExtend)*RightVector) );
				const double H = Root->GetGroundHeight(SampleLocation);
				const FVector Dir = (SampleLocation-Root->GetActorLocation()).GetSafeNormal();
				Samples.Add(Root->GetActorLocation() + ( Dir*(Root->PlanetScale+H)));
			}
		}
		return Samples;
	}
	return TArray<FVector>();
}
// ---------------------------------------------------
void UWS_FunctionLibrary::WS_Sample_2DGrid_Async(const int32 Index , FWS_SampleReturn Out , AWorldScapeRoot* Root , const FVector Center , const FRotator Rotation , const FVector Extend , const int32 Step )
{
	AsyncTask( ENamedThreads::AnyBackgroundThreadNormalTask, [ Index, Out ,Root , Center , Rotation , Extend , Step]
	{
		TArray<FVector> Samples = WS_Sample_2DGrid( Root , Center , Rotation , Extend , Step );
		AsyncTask(ENamedThreads::GameThread, [Index, Out , OutData = MoveTemp(Samples) ]() 
		{
			Out.ExecuteIfBound(Index,OutData);
		});
	});
}
// ---------------------------------------------------
bool UWS_FunctionLibrary::WS_SampleSurface(AWorldScapeRoot* Root, const FVector Location, const double Size, const double StepSize, const double HeighTreshold , TArray<FVector>& DrawDebug)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SampleSurface);
	if (IsValid(Root))
	{
		TArray<double> H_Sample;
		double H_Count = 0.0f;
		// Make Grid Data
		const FVector DirFromlCenter = (Location - Root->GetActorLocation()).GetSafeNormal();
		const FRotator BaseRotate = FRotationMatrix::MakeFromZ(DirFromlCenter).Rotator();
		const FVector Forward = BaseRotate.Vector();
		const FVector Right = FRotationMatrix(BaseRotate).GetScaledAxis(EAxis::Y);
		
		const int32 Step = FMath::Floor(Size / StepSize);
        if (Step > (2147483647 / 2))
        {
            return false;
        }
		const FVector OffsetStart = ((((Step / 2) * Forward) * -1) + (((Step / 2) * Right) * -1)) + Location;
        double HBase = Root->GetGroundHeight(Location, false);
		// Grid Sampling Ground
		for (int32 x = 0; x < Step; x++)
		{
			for (int32 y = 0; y < Step; y++)
			{
				FVector Test = OffsetStart + (x * Forward * StepSize) + (y * Right * StepSize);
				double H = Root->GetGroundHeight(Test, false);
				H_Sample.Add(H);
				H_Count += H;
                DrawDebug.Add(Test - FRotationMatrix(BaseRotate).GetScaledAxis(EAxis::Z) * (HBase - H));
			}
		}
		if ((FMath::Max(H_Sample) - FMath::Min(H_Sample)) < HeighTreshold)
		{
			return true;
		}
	}
	return false;
}
// ---------------------------------------------------
void UWS_FunctionLibrary::WS_SampleSurfaceH(AWorldScapeRoot* Root, const FVector CenterLocation, const FVector Forward, const FVector Right, const FVector Up, const double Size, const double StepSize, TArray<FVector>& DrawDebugPoints, double& HDifference, double& Median , double& moyenne)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SampleSurfaceH);
    if (IsValid(Root))
    {
        TArray<double> H_Sample;
        double H_Count = 0.0f;
        const FVector OffsetStart = (((Size / 2) * Forward) * -1) + (((Size / 2) * Right) * -1) + CenterLocation;
        const int32 Step = Size / StepSize;
        double HBase = Root->GetGroundHeight(CenterLocation, false);
        for (int32 x = 0; x < Step; x++)
        {
            for (int32 y = 0; y < Step; y++)
            {
                FVector TestLocation = OffsetStart + (x * Forward * StepSize) + (y * Right * StepSize);
                double H = Root->GetGroundHeight(TestLocation, false);
                H_Sample.Add(H);
                H_Count += H;
                DrawDebugPoints.Add(TestLocation - (Up * (HBase-H)));
            }
        }
        HDifference = FMath::Max(H_Sample) - FMath::Min(H_Sample);
        if (H_Sample.IsValidIndex(H_Sample.Num() / 2))
        {
            Median = H_Sample[H_Sample.Num() / 2] - FMath::Min(H_Sample);
        }
        moyenne = (H_Count / H_Sample.Num()) - FMath::Min(H_Sample);
    }
}
// ---------------------------------------------------
bool UWS_FunctionLibrary::WS_SampleSlopeSurface(AWorldScapeRoot* Root, const FVector Location, const double Size, const int32 Step, const double SlopeMin, const double SlopeMax, FVector& RelativeNormal)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SampleSlopeSurface);
    if (Step > 65535)
    {
        return false;
    }
    TArray<FVector> NormalSampling;

    const FVector DirFromCenter = (Location - Root->GetActorLocation()).GetSafeNormal();
    const FRotator BaseRotate = FRotationMatrix::MakeFromZ(DirFromCenter).Rotator();
    const FVector Forward = BaseRotate.Vector();
    const FVector Right = FRotationMatrix(BaseRotate).GetScaledAxis(EAxis::Y);

    const double StepSize = Size / (Step - 1);
    const FVector OffsetStart = ((((Step - 1) * 0.5f * Forward) * -1) + (((Step - 1) * 0.5f * Right) * -1)) * StepSize + Location;

    for (int32 x = 0; x < Step; x++)
    {
        for (int32 y = 0; y < Step; y++)
        {
            FVector CurrentPos = OffsetStart + (Forward * x * StepSize) + (Right * y * StepSize);

            // Projeter le point sur la sphère
            FVector SpherePos = (CurrentPos - Root->GetActorLocation()).GetSafeNormal() * (Location - Root->GetActorLocation()).Size();
            SpherePos += Root->GetActorLocation();

            // Calculer la normale pour ce point
            FVector Normal = (SpherePos - Root->GetActorLocation()).GetSafeNormal();

            // Calculer deux points adjacents pour former un triangle
            FVector AdjacentPos1 = OffsetStart + (Forward * (x + 1) * StepSize) + (Right * y * StepSize);
            FVector AdjacentPos2 = OffsetStart + (Forward * x * StepSize) + (Right * (y + 1) * StepSize);

            FVector SphereAdjacentPos1 = (AdjacentPos1 - Root->GetActorLocation()).GetSafeNormal() * (Location - Root->GetActorLocation()).Size();
            SphereAdjacentPos1 += Root->GetActorLocation();
            FVector SphereAdjacentPos2 = (AdjacentPos2 - Root->GetActorLocation()).GetSafeNormal() * (Location - Root->GetActorLocation()).Size();
            SphereAdjacentPos2 += Root->GetActorLocation();

            // Calculer la normale du triangle
            FVector TriangleNormal = FVector::CrossProduct(SphereAdjacentPos1 - SpherePos, SphereAdjacentPos2 - SpherePos).GetSafeNormal();

            NormalSampling.Add(TriangleNormal);

            // Vérifier si la pente est dans la plage acceptable
            float Slope = FMath::Abs(FVector::DotProduct(TriangleNormal, Normal));
            if (Slope < SlopeMin || Slope > SlopeMax)
            {
                return false;
            }
        }
    }

    // Calculer la normale moyenne
    FVector SumNormal = FVector::ZeroVector;
    for (const FVector& Normal : NormalSampling)
    {
        SumNormal += Normal;
    }
    RelativeNormal = SumNormal.GetSafeNormal();

    return true;
}
// ---------------------------------------------------
void UWS_FunctionLibrary::WS_SampleSlope(AWorldScapeRoot* Root, const FVector CenterLocation, const FVector Forward, const FVector Right, const FVector Up, const double Size, TArray<FVector>& DrawDebugPoints, FVector& Normal1, FVector& Normal2)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SampleSlope);
    if (IsValid(Root))
    {
        double HBase = Root->GetGroundHeight(CenterLocation, false);
        const FVector OffsetStart = (((Size / 2) * Forward) * -1) + (((Size / 2) * Right) * -1) + CenterLocation;
        // FirstStep
        FVector TO = OffsetStart;
        FVector T1 = OffsetStart + (Forward * Size);
        FVector T2 = OffsetStart + (Right * Size);
        FVector A0 = OffsetStart + (Forward * Size) + (Right * Size);

        FVector G1 = FVector(TO.X, TO.Y, Root->GetGroundHeight(TO, false));
        FVector G2 = FVector(T1.X, T1.Y, Root->GetGroundHeight(T1, false));
        FVector G3 = FVector(T2.X, T2.Y, Root->GetGroundHeight(T2, false));
        FVector G4 = FVector(A0.X, A0.Y, Root->GetGroundHeight(A0, false));

        Normal1 = FVector::CrossProduct(G2 - G1, G3 - G1).GetSafeNormal();
        Normal2 = FVector::CrossProduct(G2 - G4, G3 - G4).GetSafeNormal();

        DrawDebugPoints.Add(FVector(TO - (Up * (HBase - G1.Z))));
        DrawDebugPoints.Add(FVector(T1 - (Up * (HBase - G2.Z))));
        DrawDebugPoints.Add(FVector(T2 - (Up * (HBase - G3.Z))));
        DrawDebugPoints.Add(FVector(A0 - (Up * (HBase - G4.Z))));
    }
}
// ---------------------------------------------------
FVector UWS_FunctionLibrary::WS_SphereToCube(const FVector& Location, const FVector& CenterLocation, const double MinRadius)
{
    // Calculer la direction normalisée du centre à la location
    FVector Direction = (Location - CenterLocation).GetSafeNormal();

    // Trouver quelle face du cube est touchée
    double AbsX = FMath::Abs(Direction.X);
    double AbsY = FMath::Abs(Direction.Y);
    double AbsZ = FMath::Abs(Direction.Z);

    double MaxComponent = FMath::Max3(AbsX, AbsY, AbsZ);

    FVector Result;

    // Projeter sur la face du cube appropriée
    if (MaxComponent == AbsX)
    {
        // Face X
        Result = FVector(
            FMath::Sign(Direction.X),
            Direction.Y / AbsX,
            Direction.Z / AbsX
        );
    }
    else if (MaxComponent == AbsY)
    {
        // Face Y
        Result = FVector(
            Direction.X / AbsY,
            FMath::Sign(Direction.Y),
            Direction.Z / AbsY
        );
    }
    else
    {
        // Face Z
        Result = FVector(
            Direction.X / AbsZ,
            Direction.Y / AbsZ,
            FMath::Sign(Direction.Z)
        );
    }

    // Mettre à l'échelle en fonction du rayon minimum
    Result *= MinRadius;

    // Ajouter la position du centre pour obtenir la position mondiale
    Result += CenterLocation;

    return Result;
}
// ---------------------------------------------------
FWS_Grid UWS_FunctionLibrary::WS_GetGridCoord( const FVector& Location,const FVector& BaseLocation , const int32 Depth, const double Radius)
{
	return FWS_Grid( BaseLocation ,Location , Depth , Radius );
}
// ---------------------------------------------------
TArray<FWS_Grid> UWS_FunctionLibrary::WS_GetAdjacentGridCoord( const FWS_Grid& Coord ,const FVector& BaseLocation , const double Radius)
{
	return Coord.Get_Adjacent_Cells(BaseLocation,Radius);
}
// ---------------------------------------------------
void WS_Spherical_OctreeGrid(const FVector CenterLocation, const double MinRadius, const double MaxRadius, const int32 HGrid, const int32 SubDivision)
{

}
// ---------------------------------------------------
TArray<FTransform> UWS_FunctionLibrary::WS_ProjectGroundLocation_Multi(AWorldScapeRoot* Root, const TArray<FVector>& Locations )
{
	if ( IsValid(Root) )
	{
		TArray<FTransform> Out;
		TArray<double> Result = Root->GetGroundHeight_HOnly_Multi(Locations);
		for ( int i = 0; i < Locations.Num(); i++ )
		{
			const FVector Dir = (Root->GetActorLocation()-Locations[i]).GetSafeNormal();
			Out.Add(FTransform(FRotationMatrix::MakeFromZ(Dir).Rotator(), Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[i]))));
		}
		return Out;
	}
	return TArray<FTransform>();
}
// ---------------------------------------------------
FVector UWS_FunctionLibrary::WS_GetTerrainNormalTriTest(AWorldScapeRoot* Root, const FVector& Location , const float Offset )
{
	if ( IsValid(Root) )
	{
		const FVector Dir = (Root->GetActorLocation()-Location).GetSafeNormal();
		const FVector A = FVector(1,0,0).RotateAngleAxis(0.0, Dir.GetSafeNormal());
		const FVector B = FVector(1,0,0).RotateAngleAxis(120.0, Dir.GetSafeNormal());
		const FVector C = FVector(1,0,0).RotateAngleAxis(-120.0, Dir.GetSafeNormal());

		const FVector T0 = Root->GetActorLocation()+(Dir*(Root->PlanetScale + Root->GetGroundHeight_HOnly(Location + A*Offset)));
		const FVector T1 = Root->GetActorLocation()+(Dir*(Root->PlanetScale + Root->GetGroundHeight_HOnly(Location + B*Offset)));
		const FVector T2 = Root->GetActorLocation()+(Dir*(Root->PlanetScale + Root->GetGroundHeight_HOnly(Location + C*Offset)));
		
		return FVector::CrossProduct(T1 - T0, T2 - T0).GetSafeNormal();
	}
	return FVector();
}
// ---------------------------------------------------
TArray<FTransform> UWS_FunctionLibrary::WS_ProjectGroundLocationWithNormal_Multi(AWorldScapeRoot* Root, const TArray<FVector>& Locations , const float Offset )
{
	if ( IsValid(Root) )
	{
		TArray<FVector> TestLocation;
		for ( const auto& Location : Locations )
		{
			TestLocation.Add(Location);
			const FVector Dir = (Root->GetActorLocation()-Location).GetSafeNormal();
			TestLocation.Add(Location+(FVector(1,0,0).RotateAngleAxis(0.0, Dir.GetSafeNormal())*Offset));
			TestLocation.Add(Location+(FVector(1,0,0).RotateAngleAxis(120.0, Dir.GetSafeNormal())*Offset));
			TestLocation.Add(Location+(FVector(1,0,0).RotateAngleAxis(-120.0, Dir.GetSafeNormal())*Offset));
		}
		TArray<FTransform> Out;
		TArray<double> Result = Root->GetGroundHeight_HOnly_Multi(TestLocation);
		for ( int i = 0; i < Result.Num(); i+=4 )
		{
			const FVector Dir = (Root->GetActorLocation()-TestLocation[i]).GetSafeNormal();
			const FVector T0 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[i+1]));
			const FVector T1 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[i+2]));
			const FVector T2 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[i+3]));
			const FVector Normal = FVector::CrossProduct(T1 - T0, T2 - T0).GetSafeNormal();
			Out.Add(FTransform(FRotationMatrix::MakeFromZ(Normal).Rotator(), Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[i])),Dir));
		}
		return Out;
	}
	return TArray<FTransform>();
}
// ---------------------------------------------------
void UWS_FunctionLibrary::WS_ProjectActorToGround(AWorldScapeRoot* Root, AActor* Actor , const double Offset )
{
	if ( IsValid(Root) && IsValid(Actor) )
	{
		FVector Origin;
		FVector Extend;
		Actor->GetActorBounds(true , Origin , Extend);

		TArray<FVector> TestLocation;
		TestLocation.Add(Origin);
		const FVector Dir = (Root->GetActorLocation()-Origin).GetSafeNormal();
		TestLocation.Add(Origin+(FVector(1,0,0).RotateAngleAxis(0.0, Dir.GetSafeNormal())*Offset));
		TestLocation.Add(Origin+(FVector(1,0,0).RotateAngleAxis(120.0, Dir.GetSafeNormal())*Offset));
		TestLocation.Add(Origin+(FVector(1,0,0).RotateAngleAxis(-120.0, Dir.GetSafeNormal())*Offset));
		TArray<double> Result = Root->GetGroundHeight_HOnly_Multi(TestLocation);
		if ( Result.Num() == 4 )
		{
			const FVector OutLocation = Root->GetActorLocation() + (Dir * (Root->PlanetScale + Result[0] + Extend.Z + Offset));
			const FVector T0 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[1]));
			const FVector T1 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[2]));
			const FVector T2 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[3]));
			const FVector Normal = FVector::CrossProduct(T1 - T0, T2 - T0).GetSafeNormal();
			Actor->SetActorLocationAndRotation(OutLocation,FRotationMatrix::MakeFromZX(Normal, Actor->GetActorForwardVector()).Rotator());
		}
	}
}
// ---------------------------------------------------
FTransform UWS_FunctionLibrary::WS_ProjectTransformToGround(AWorldScapeRoot* Root, const FTransform& Transform, const double Offset )
{
	if ( IsValid(Root) )
	{
		const FVector Origin = Transform.GetLocation();
		const FVector Extend = Transform.GetScale3D();
		
		TArray<FVector> TestLocation;
		TestLocation.Add(Origin);
		//const FVector Dir = (Root->GetActorLocation()-Origin).GetSafeNormal();
		const FVector Dir = (Origin-Root->GetActorLocation()).GetSafeNormal();
		
		TestLocation.Add(Origin+(FVector(1,0,0).RotateAngleAxis(1.0, Dir.GetSafeNormal())*Offset));
		TestLocation.Add(Origin+(FVector(1,0,0).RotateAngleAxis(120.0, Dir.GetSafeNormal())*Offset));
		TestLocation.Add(Origin+(FVector(1,0,0).RotateAngleAxis(-120.0, Dir.GetSafeNormal())*Offset));
		TArray<double> Result = Root->GetGroundHeight_HOnly_Multi(TestLocation);
		if ( Result.Num() == 4 )
		{
			const FVector OutLocation = Root->GetActorLocation() + (Dir * (Root->PlanetScale + Result[0] + Extend.Z + Offset));
			const FVector T0 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[1]));
			const FVector T1 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[2]));
			const FVector T2 = Root->GetActorLocation()+(Dir*(Root->PlanetScale+Result[3]));
			const FVector Normal = FVector::CrossProduct(T1 - T0, T2 - T0).GetSafeNormal();

			const double test = Root->GetGroundHeight(Transform.GetLocation());
			const FVector OutLoc = Root->GetActorLocation() + (Dir * (Root->PlanetScale + test));
			FTransform OutTransform;
			OutTransform.SetLocation(OutLoc);
			//OutTransform.SetLocation(OutLocation);
			OutTransform.SetRotation(FRotationMatrix::MakeFromZX(Normal, Transform.GetRotation().GetForwardVector()).ToQuat());
			return OutTransform;
		}
	}
	return Transform;
}
// ---------------------------------------------------
FVector UWS_FunctionLibrary::WS_SingleProject(AWorldScapeRoot* Root, const FVector& Location )
{
	if ( IsValid(Root) )
	{
		const double H = Root->GetGroundHeight(Location);
		const FVector Dir = (Location-Root->GetActorLocation()).GetSafeNormal();
		return Root->GetActorLocation() + ( Dir*(Root->PlanetScale+H));
	}
	return FVector(0,0,0);
}
// ---------------------------------------------------
FTransform UWS_FunctionLibrary::WS_SingleProject_Transform(AWorldScapeRoot* Root, const FVector& Location , const double Size , const double YawBase , const double Z_Offset )
{
	if ( IsValid(Root) )
	{
		FWS_Grid Grid = FWS_Grid();
		Grid.SizeData.A = Location-FVector(1,1,0);
		Grid.SizeData.B = Location+FVector(1,1,0);
		return Root->Grid_Math_GetTransformNormalAt( Location , Size ,YawBase , Z_Offset , FVector(1,0,0) , false,Root->Grid_GetWSVolume( Grid ));
	}
	return FTransform();
}
// ---------------------------------------------------
double UWS_FunctionLibrary::WS_GetHeightValueAtLocation(AWorldScapeRoot* Root, const FVector& Location )
{
	if ( IsValid(Root) )
	{
		return Root->GetGroundNoise(Location, false).Height;
	}
	return -1;
}
// ---------------------------------------------------
double UWS_FunctionLibrary::WS_GetTemperatureValueAtLocation(AWorldScapeRoot* Root, const FVector& Location )
{
	if ( IsValid(Root) )
	{
		return Root->GetGroundNoise(Location, false).Temperature;
	}
	return -1;
}
// ---------------------------------------------------
double UWS_FunctionLibrary::WS_GetHumidityValueAtLocation(AWorldScapeRoot* Root, const FVector& Location )
{
	if ( IsValid(Root) )
	{
		return Root->GetGroundNoise(Location, false).Humidity;
	}
	return -1;
}
// ---------------------------------------------------
bool UWS_FunctionLibrary::WS_GetGroundInWater(AWorldScapeRoot* Root, const FVector& Location)
{
	if ( IsValid(Root) )
	{
		const double Ground = Root->GetGroundHeight(Location, false);
		const double Water = Root->GetGroundHeight(Location, true);
		if ( Ground > Water )
		{
			return false;
		}
	}
	return true;
}

// --------------------------------------
TArray<uint8> UWS_FunctionLibrary::Grid_SerializeActor_Direct( AActor* Actor )
{
	if ( IsValid(Actor) )
	{
		TArray<uint8> Data;
		FMemoryWriter Ar(Data, true);
		FObjectAndNameAsStringProxyArchive Archive(Ar, true);
		Archive.ArIsSaveGame = true;
		Archive.ArIgnoreOuterRef = true;
		//Ar.ArNoDelta = true;
		Actor->Serialize(Archive);
		//Actor->SerializeScriptProperties(Archive);
		Archive.Close();
		return Data;
	}
	return TArray<uint8>();
}
// --------------------------------------
double UWS_FunctionLibrary::WS_FindMinimalGroundH( AWorldScapeRoot* Root , const TArray<FVector>& Points )
{
	if ( IsValid(Root) )
	{
		double SquaredLength = FMath::Square(Root->PlanetScale*2.0);
		for ( auto& Point : Points )
		{
			const double Length = (Root->GetActorLocation()-Point).SquaredLength();
			if ( Length < SquaredLength )
			{
				SquaredLength = Length;
			}
		}
		return FMath::Sqrt(SquaredLength);
	}
	return 0.0f;
}
// --------------------------------------
FVector UWS_FunctionLibrary::WS_OverrideLocationH( AWorldScapeRoot* Root , const FVector& Location , const double NewH )
{
	if ( IsValid(Root) )
	{
		const FVector Dir = (Location-Root->GetActorLocation()).GetSafeNormal();
		return Root->GetActorLocation() + Dir*NewH;
	}
	return Location;
}
// --------------------------------------
FVector UWS_FunctionLibrary::WS_CubicBezierY(const FVector& A, const FVector& B, const FVector& C, const FVector& D, float T)
{
	FVector AB = FMath::Lerp(A, B, T);
	FVector BC = FMath::Lerp(B, C, T);
	FVector CD = FMath::Lerp(C, D, T);

	FVector AB_BC = FMath::Lerp(AB, BC, T);
	FVector BC_CD = FMath::Lerp(BC, CD, T);

	return FMath::Lerp(AB_BC, BC_CD, T);
}
// --------------------------------------
FVector UWS_FunctionLibrary::WS_GetSplineOffset(const FVector& A, const FVector& B, const FVector& C, const FVector& D, float T)
{
	FVector StraightPos = FMath::Lerp(A, D, T);
	FVector SplinePos = WS_CubicBezierY(A, B, C, D, T);
	return SplinePos - StraightPos;
}

// --------------------------------------
FTransform UWS_FunctionLibrary::WS_RelativeToWorld( const FTransform& WorldTransform , const FVector& RelativeLocation , const FRotator& RelativeRotation )
{
	FTransform Out;
	Out.SetLocation( WorldTransform.TransformPosition(RelativeLocation) );
	Out.SetRotation( WorldTransform.TransformRotation(RelativeRotation.Quaternion()) );
	return Out;
}
// --------------------------------------
double UWS_FunctionLibrary::WS_GetAltitudeInLocation( AWorldScapeRoot* Root , const FVector& Location , const double Offset )
{
	if ( IsValid(Root) )
	{
		return (Root->GetActorLocation() - Location).Length() - Root->PlanetScale + Offset;
	}
	return -1.0;
}
// --------------------------------------
TArray<int> UWS_FunctionLibrary::WS_GetAllFoliageIndexMask( AWorldScapeRoot* Root )
{
	if ( IsValid(Root) )
	{
		TSet<int> Mask;
		for ( auto& Fol : Root->Foliages )
		{
			if ( IsValid(Fol) )
			{
				Mask.Add(Fol->FoliageLayer);
				for (auto& Sub : Fol->FoliageList )
				{
					if ( IsValid(Sub) )
					{
						Mask.Add(Sub->FoliageLayer);
					}
				}
			}
		}
		return Mask.Array();
	}
	return TArray<int>();
}