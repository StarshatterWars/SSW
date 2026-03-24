#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "SSWGameInstance.h"
#include "TimerSubsystem.h"
#include "UniverseSaveGame.h"
#include "DataLoader.h"
#include "GameLoader.generated.h"

UCLASS()
class STARSHATTERWARS_API AGameLoader : public ALevelScriptActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	AGameLoader();

	UFUNCTION(BlueprintCallable, Category = "Game Loader")
	USSWGameInstance* GetSSWGameInstance();

	UFUNCTION(BlueprintCallable, Category = "Game Loader")
	void LoadGalaxy();

	void InitializeGame();
	void LoadMainMenu();

	// NEW: Universe seed bootstrap
	void LoadOrCreateUniverse();

	static DataLoader* loader;

	UPROPERTY(EditAnywhere, Category = "UI Sound")
	USoundBase* MenuMusic;

private:
	FString GetUniverseSlotName() const { return TEXT("Universe_Main"); }

	UPROPERTY()
	TObjectPtr<UUniverseSaveGame> UniverseSave;
};
