#include "SSWSessionLobbyGameMode.h"
#include "MultiplayerDlg.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/GameState.h"
#include "GameFramework/GameSession.h"
#include "SSWRuntimeSubsystem.h"
#include "Engine/GameInstance.h"
ASSWSessionLobbyGameMode::ASSWSessionLobbyGameMode(){PlayerControllerClass=ASSWSessionLobbyController::StaticClass();DefaultPawnClass=ASpectatorPawn::StaticClass();GameStateClass=AGameState::StaticClass();GameSessionClass=AGameSession::StaticClass();bUseSeamlessTravel=true;}
void ASSWSessionLobbyController::BeginPlay(){Super::BeginPlay();if(!IsLocalController())return;
 if(auto* Runtime=GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())Runtime->SetPaused(true);
 if(auto* Dialog=CreateWidget<UMultiplayerDlg>(this,UMultiplayerDlg::StaticClass())){Dialog->AddToViewport(300);Dialog->Show();bShowMouseCursor=true;FInputModeUIOnly Input;Input.SetWidgetToFocus(Dialog->TakeWidget());SetInputMode(Input);}
}
