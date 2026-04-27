#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "SystemLevelScriptActor.generated.h"

class ASystemSceneBuilder;
class ACampaignSceneActor;

UCLASS()
class STARSHATTERWARS_API ASystemLevelScriptActor : public ALevelScriptActor
{
    GENERATED_BODY()

public:
    ASystemLevelScriptActor();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "System")
    void InitializeSystemLevel();

    UFUNCTION(BlueprintCallable, Category = "System")
    void BuildCurrentSystem();

    UFUNCTION(BlueprintCallable, Category = "System")
    void ClearCurrentSystem();

    UFUNCTION(BlueprintCallable, Category = "System")
    void RebuildCurrentSystem();

    UFUNCTION(BlueprintCallable, Category = "System")
    FString ResolveStartupSystemName() const;

    UFUNCTION(BlueprintPure, Category = "System")
    ASystemSceneBuilder* GetSystemSceneBuilder() const
    {
        return CachedBuilder.Get();
    }

    UFUNCTION(BlueprintPure, Category = "Scene")
    ACampaignSceneActor* GetCampaignSceneActor() const
    {
        return CachedSceneActor.Get();
    }

protected:
    ASystemSceneBuilder* ResolveBuilder();
    ACampaignSceneActor* ResolveSceneActor();

protected:
    UPROPERTY(EditInstanceOnly, Category = "System")
    bool bInitializeOnBeginPlay = true;

    UPROPERTY(EditInstanceOnly, Category = "System")
    bool bBuildOnBeginPlay = true;

    UPROPERTY(EditInstanceOnly, Category = "System")
    bool bAutoFindBuilder = true;

    UPROPERTY(EditInstanceOnly, Category = "System")
    bool bAutoSpawnBuilderIfMissing = false;

    UPROPERTY(EditInstanceOnly, Category = "System")
    TSubclassOf<ASystemSceneBuilder> BuilderClass;

    UPROPERTY(EditInstanceOnly, Category = "System")
    FVector BuilderSpawnLocation = FVector::ZeroVector;

    UPROPERTY(EditInstanceOnly, Category = "System")
    FRotator BuilderSpawnRotation = FRotator::ZeroRotator;

    UPROPERTY(EditInstanceOnly, Category = "System")
    TObjectPtr<ASystemSceneBuilder> BuilderOverride = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "System")
    FString DefaultSystemName = TEXT("Solus");

protected:
    UPROPERTY(EditInstanceOnly, Category = "Scene")
    bool bAutoFindSceneActor = true;

    UPROPERTY(EditInstanceOnly, Category = "Scene")
    bool bAutoSpawnSceneActorIfMissing = true;

    UPROPERTY(EditInstanceOnly, Category = "Scene")
    TSubclassOf<ACampaignSceneActor> SceneActorClass;

    UPROPERTY(EditInstanceOnly, Category = "Scene")
    FVector SceneActorSpawnLocation = FVector::ZeroVector;

    UPROPERTY(EditInstanceOnly, Category = "Scene")
    FRotator SceneActorSpawnRotation = FRotator::ZeroRotator;

    UPROPERTY(EditInstanceOnly, Category = "Scene")
    TObjectPtr<ACampaignSceneActor> SceneActorOverride = nullptr;

protected:
    UPROPERTY(VisibleInstanceOnly, Category = "Runtime")
    TObjectPtr<ASystemSceneBuilder> CachedBuilder = nullptr;

    UPROPERTY(VisibleInstanceOnly, Category = "Runtime")
    TObjectPtr<ACampaignSceneActor> CachedSceneActor = nullptr;
};