#pragma once
#include "CoreMinimal.h"
struct FS_Galaxy;
struct FRegion;
/** Native catalog IO. Compatibility tables are populated only after validation. */
namespace GalaxyJsonCatalog
{
bool Load(const FString& Path,TArray<FS_Galaxy>& Systems,TArray<FRegion>& Regions,FString& Error);
bool Save(const FString& Path,const TArray<FS_Galaxy>& Systems,const TArray<FRegion>& Regions,FString& Error);
bool Parse(const FString& Json,TArray<FS_Galaxy>& Systems,TArray<FRegion>& Regions,FString& Error);
}
