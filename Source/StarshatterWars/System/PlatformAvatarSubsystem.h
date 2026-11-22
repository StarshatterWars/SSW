#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Styling/SlateBrush.h"
#include "PlatformAvatarSubsystem.generated.h"

class UTexture2D;

// Presentation cache only. Platform identity and simulation ownership remain separate.
UCLASS()
class STARSHATTERWARS_API UPlatformAvatarSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void SetAccount(FName Provider, const FString& AccountId);
    void Refresh();
    const FSlateBrush* GetBrush() const { return Texture ? &Brush : nullptr; }
    FText GetStatus() const;
private:
    FName AccountProvider;
    FString Account;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Texture;
    FSlateBrush Brush;
    int32 Attempts = 0;
};
