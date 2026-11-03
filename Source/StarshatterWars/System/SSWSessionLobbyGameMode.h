#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerController.h"
#include "SSWSessionLobbyGameMode.generated.h"
UCLASS()
class STARSHATTERWARS_API ASSWSessionLobbyController:public APlayerController {
 GENERATED_BODY()
protected: virtual void BeginPlay()override;
};
// Networked staging lobby only. Legacy combat replication is a separate stage.
UCLASS()
class STARSHATTERWARS_API ASSWSessionLobbyGameMode:public AGameMode {
 GENERATED_BODY()
public: ASSWSessionLobbyGameMode();
};
