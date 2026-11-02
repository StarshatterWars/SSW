#pragma once
#include "Commandlets/Commandlet.h"
#include "SSWWeaponProjectileImportCommandlet.generated.h"
UCLASS()
class STARSHATTERWARS_API USSWWeaponProjectileImportCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    USSWWeaponProjectileImportCommandlet();
    virtual int32 Main(const FString& Params) override;
};
