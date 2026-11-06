// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapeRoot.h"

// -------------------------------------------------------
// Gravity
// -------------------------------------------------------
FVector AWorldScapeRoot::WS_GetGravityFromLocation( const FVector& Location , FVector& GravityDir , double& Force )
{
	if ( GenerationType == EWorldScapeType::Planet )
	{
		const double Distance = (GetActorLocation()-Location).SquaredLength();
		GravityDir = (GetActorLocation()-Location).GetSafeNormal();
		Force = FMath::GetMappedRangeValueClamped(FVector2D(FMath::Square(PlanetScale*Gravity_Planetary_StartFade),FMath::Square(PlanetScale*Gravity_Planetary_EndFadeScale)),FVector2D(Gravity_Force,0.0f),Distance);
		return GravityDir*Force;
	}
	else
	{
		GravityDir = GetActorUpVector() * -1.0f;
		const double Distance = GetPawnDistanceFromGround(Location);
		Force = FMath::GetMappedRangeValueClamped(FVector2D(Gravity_Flat_StartFadeDistance,Gravity_Flat_EndFadeDistance),FVector2D(Gravity_Force,0.0f),Distance);
		return GravityDir*Force;
	}
}
#if WITH_EDITOR
void AWorldScapeRoot::Gravity_Editor_Update()
{
	if ( Gravity_ViewDebug )
	{
		if ( GenerationType == EWorldScapeType::Planet )
		{
			Gravity_Editor_DestroyBox();
			Gravity_Editor_CreateSphere();
		}
		else
		{
			Gravity_Editor_DestroySphere();
			Gravity_Editor_CreateBox();
		}
	}
	else
	{
		Gravity_Editor_Clear();
	}
}
void AWorldScapeRoot::Gravity_Editor_Clear()
{
	Gravity_Editor_DestroySphere();
	Gravity_Editor_DestroyBox();
}
// Sphere
void AWorldScapeRoot::Gravity_Editor_CreateSphere()
{
	if ( !IsValid(Gravity_Sphere_Start) )
	{
		EObjectFlags inObjectFlags = RF_Transient;
		Gravity_Sphere_Start = NewObject<USphereComponent>(this , USphereComponent::StaticClass(),TEXT("Editor_SphereGravity_Start"),inObjectFlags);
		Gravity_Sphere_Start->SetComponentTickEnabled(false);
		Gravity_Sphere_Start->bIsEditorOnly = true;
		Gravity_Sphere_Start->SetGenerateOverlapEvents(false);
		Gravity_Sphere_Start->SetMobility(EComponentMobility::Static);
		Gravity_Sphere_Start->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Gravity_Sphere_Start->SetCollisionProfileName(TEXT("NoCollision"));
		Gravity_Sphere_Start->SetupAttachment(GetRootComponent());
		Gravity_Sphere_Start->SetSphereRadius(PlanetScale*Gravity_Planetary_StartFade);
		Gravity_Sphere_Start->ShapeColor = FColor::Blue;
		Gravity_Sphere_Start->RegisterComponent();
	}
	if ( !IsValid(Gravity_Sphere_End) )
	{
		EObjectFlags inObjectFlags = RF_Transient;
		Gravity_Sphere_End = NewObject<USphereComponent>(this , USphereComponent::StaticClass(),TEXT("Editor_SphereGravity_End"),inObjectFlags);
		Gravity_Sphere_End->SetComponentTickEnabled(false);
		Gravity_Sphere_End->bIsEditorOnly = true;
		Gravity_Sphere_End->SetGenerateOverlapEvents(false);
		Gravity_Sphere_End->SetMobility(EComponentMobility::Static);
		Gravity_Sphere_End->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Gravity_Sphere_End->SetCollisionProfileName(TEXT("NoCollision"));
		Gravity_Sphere_End->SetupAttachment(GetRootComponent());
		Gravity_Sphere_End->SetSphereRadius(PlanetScale*Gravity_Planetary_EndFadeScale);
		Gravity_Sphere_End->ShapeColor = FColor::Red;
		Gravity_Sphere_End->RegisterComponent();
	}
	Gravity_Editor_UpdateSphere();
}
void AWorldScapeRoot::Gravity_Editor_UpdateSphere()
{
	if ( IsValid(Gravity_Sphere_Start) )
	{
		Gravity_Sphere_Start->SetSphereRadius(PlanetScale*Gravity_Planetary_StartFade);
	}
	if ( IsValid(Gravity_Sphere_End) )
	{
		Gravity_Sphere_End->SetSphereRadius(PlanetScale*Gravity_Planetary_EndFadeScale);
	}
}
void AWorldScapeRoot::Gravity_Editor_DestroySphere()
{
	if ( IsValid(Gravity_Sphere_Start) )
	{
		Gravity_Sphere_Start->DestroyComponent();
		Gravity_Sphere_Start = nullptr;
	}
	if ( IsValid(Gravity_Sphere_End) )
	{
		Gravity_Sphere_End->DestroyComponent();
		Gravity_Sphere_End = nullptr;
	}
}
// Box
void AWorldScapeRoot::Gravity_Editor_CreateBox()
{
	if ( !IsValid(Gravity_Box_Start) )
	{
		EObjectFlags inObjectFlags = RF_Transient;
		Gravity_Box_Start = NewObject<UBoxComponent>(this , UBoxComponent::StaticClass(),TEXT("Editor_BoxGravity_Start"),inObjectFlags);
		Gravity_Box_Start->SetComponentTickEnabled(false);
		Gravity_Box_Start->bIsEditorOnly = true;
		Gravity_Box_Start->SetGenerateOverlapEvents(false);
		Gravity_Box_Start->SetMobility(EComponentMobility::Static);
		Gravity_Box_Start->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Gravity_Box_Start->SetCollisionProfileName(TEXT("NoCollision"));
		Gravity_Box_Start->SetupAttachment(RootComponent);
		Gravity_Box_Start->SetBoxExtent(Gravity_Editor_UpdateSize( false ));
		Gravity_Box_Start->ShapeColor = FColor::Blue;
		Gravity_Box_Start->RegisterComponent();
	}
	if ( !IsValid(Gravity_Box_End) )
	{
		EObjectFlags inObjectFlags = RF_Transient;
		Gravity_Box_End = NewObject<UBoxComponent>(this , UBoxComponent::StaticClass(),TEXT("Editor_BoxGravity_End"),inObjectFlags);
		Gravity_Box_End->SetComponentTickEnabled(false);
		Gravity_Box_End->bIsEditorOnly = true;
		Gravity_Box_End->SetGenerateOverlapEvents(false);
		Gravity_Box_End->SetMobility(EComponentMobility::Static);
		Gravity_Box_End->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Gravity_Box_End->SetCollisionProfileName(TEXT("NoCollision"));
		Gravity_Box_End->SetupAttachment(RootComponent);
		Gravity_Box_End->SetBoxExtent(Gravity_Editor_UpdateSize( true ));
		Gravity_Box_End->ShapeColor = FColor::Red;
		Gravity_Box_End->RegisterComponent();
	}
	Gravity_Editor_UpdateBox();
}
void AWorldScapeRoot::Gravity_Editor_UpdateBox()
{
	if ( IsValid(Gravity_Box_Start) )
	{
		Gravity_Box_Start->SetBoxExtent(Gravity_Editor_UpdateSize( false ));
	}
	if ( IsValid(Gravity_Box_End) )
	{
		Gravity_Box_End->SetBoxExtent(Gravity_Editor_UpdateSize( true ));
	}
}
void AWorldScapeRoot::Gravity_Editor_DestroyBox()
{
	if ( IsValid(Gravity_Box_Start) )
	{
		Gravity_Box_Start->DestroyComponent();
		Gravity_Box_Start = nullptr;
	}
	if ( IsValid(Gravity_Box_End) )
	{
		Gravity_Box_End->DestroyComponent();
		Gravity_Box_End = nullptr;
	}
}
FVector AWorldScapeRoot::Gravity_Editor_UpdateSize( const bool End )
{
	const FVector Distance = Gravity_Editor_ComputeDistance();
	if ( End )
	{
		return FVector( Distance.X*2.0 , Distance.Y*2.0 , Gravity_Flat_EndFadeDistance );
	}
	return FVector( Distance.X*2.0 , Distance.Y*2.0 , Gravity_Flat_StartFadeDistance );
}
FVector AWorldScapeRoot::Gravity_Editor_ComputeDistance()
{
	if (UWorldScapeEditorUtils::IsInViewPort())
	{
		return (UWorldScapeEditorUtils::GetViewPortCameraPosition()-GetActorLocation()).GetAbs();
	}
	else
	{
		return FVector(1000000,1000000,1000000);
	}
}
#endif