// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// =========================================================================
// Core + Engine
// =========================================================================
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/TimerHandle.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

// =========================================================================
// Kismet / Utility
// =========================================================================
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/DataTableFunctionLibrary.h"

// =========================================================================
// File / Paths / IO
// =========================================================================
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h" // (your original include spelling)
#include "HAL/PlatformFileManager.h" // (you also used this elsewhere; keep if needed)

// =========================================================================
// Data / Tables
// =========================================================================
#include "Engine/DataTable.h"
#include "GameStructs.h"

// =========================================================================
// Image Loading
// =========================================================================
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Modules/ModuleManager.h"
#include "Engine/Texture2D.h"
#include "RenderUtils.h"

// =========================================================================
// Time
// =========================================================================
#include "Misc/DateTime.h"
#include "Misc/TimeSpan.h"

// =========================================================================
// Audio
// =========================================================================
#include "Sound/SoundBase.h"

// =========================================================================
// Saves
// =========================================================================
#include "UniverseSaveGame.h"
#include "CampaignSave.h"

// =========================================================================
// Subsystems
// =========================================================================

#include "TimerSubsystem.h"
#include "MenuScreen.h"
// =========================================================================
// Generated
// =========================================================================
#include "SSWGameInstance.generated.h"


/**
 *
 */

 // =========================================================================
 // Forward Declarations
 // =========================================================================
class UUniverse;
class USim;
class AGalaxy;
class AGameDataLoader;
class AAwardInfoLoader;
class AMusicController;
class ACombatGroupLoader;
class UExitDlg;
class UMainMenu;
class UMenuDlg;
class UFirstTimeDlg;
class UCampaignScreen;
class UOperationsScreen;
class UCampaignLoading;
class UMissionLoading;
class DataLoader;
class UPlayerSaveGame;

class ASystemOverview;
class UTextureRenderTarget2D;

class UStarshatterAssetRegistrySubsystem;

// =========================================================================
// GameInstance
// =========================================================================
UCLASS()
class STARSHATTERWARS_API USSWGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	USSWGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	virtual void OnStart() override;

	// =====================================================================
	// Paths / Project
	// =====================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Variables")
	FString ProjectPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Variables")
	FString FilePath;

	UFUNCTION(BlueprintCallable, Category = "Game Variables")
	void Print(const FString& A, const FString& B);

	UFUNCTION(BlueprintCallable, Category = "Campaign UI")
	void LoadCampaignUIBundle(const FString& CampaignFolder);

	UFUNCTION(BlueprintCallable, Category = "Campaign UI")
	void ClearCampaignUIBundle();

	const FS_CampaignUIBundle& GetActiveCampaignUIBundle() const { return ActiveCampaignUIBundle; }

	// =====================================================================
	// Boot / Startup
	// =====================================================================
	UFUNCTION(BlueprintCallable, Category = "Game Variables")
	void StartGameTimers();

	UFUNCTION(BlueprintCallable, Category = "Game Variables")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game Variables")
	void SpawnGalaxy();

	// =====================================================================
	// Screen / Level Loading
	// =====================================================================

	UFUNCTION(BlueprintCallable, Category = "Game Variables")
	void LoadTransitionScreen();

	// =====================================================================
	// Show Screens
	// =====================================================================
	UFUNCTION(BlueprintCallable, Category = "Game Variables")
	void ShowMainMenuScreen();

	UFUNCTION()
	void RemoveScreens();

	// =====================================================================
	// Game Mode / Campaign Selection
	// =====================================================================
	UFUNCTION()
	void SetGameMode(EGameMode gm);

	UFUNCTION()
	EGameMode GetGameMode();

	UFUNCTION()
	void SetActiveCampaign(FS_Campaign campaign);

	UFUNCTION()
	void SetActiveCampaignNr(int active);

	UFUNCTION()
	void SetSelectedMissionNr(int active);

	UFUNCTION()
	void SetSelectedActionNr(int active);

	UFUNCTION()
	void SetSelectedRosterNr(int active);

	UFUNCTION()
	void SetCampaignActive(bool bIsActive);

	UFUNCTION()
	FS_Campaign GetActiveCampaign();

	UFUNCTION()
	int GetActiveCampaignNr();

	UFUNCTION()
	int GetSelectedMissionNr();

	UFUNCTION()
	int GetSelectedActionNr();

	UFUNCTION()
	int GetSelectedRosterNr();

	UFUNCTION()
	bool GetCampaignActive();

	// =====================================================================
	// Time: Game / Campaign
	// =====================================================================
	UFUNCTION()
	void SetGameTime(int64 time);

	UFUNCTION()
	int64 GetGameTime();

	UFUNCTION()
	void SetCampaignTime(int64 time);

	UFUNCTION()
	int64 GetCampaignTime();

	// =====================================================================
	// SaveGame (Player)
	// =====================================================================
	//UFUNCTION(BlueprintCallable, Category = "SaveGame")
	//void SaveGame(FString SlotName, int32 UserIndex, FS_PlayerGameInfo PlayerInfo);

	//UFUNCTION(BlueprintCallable, Category = "SaveGame")
	//void LoadGame(FString SlotName, int32 UserIndex);

	// =====================================================================
	// Utilities
	// =====================================================================
	UFUNCTION(BlueprintCallable, Category = "Utilities")
	UTexture2D* LoadPNGTextureFromFile(const FString& Path);

	// =====================================================================
	// Audio
	// =====================================================================
	UFUNCTION()
	void PlayMusic(USoundBase* Music);

	UFUNCTION()
	void StopMusic();

	UFUNCTION()
	void PlayMenuMusic();

	UFUNCTION()
	void PlaySoundFromFile(FString& AudioPath);

	UFUNCTION()
	bool IsSoundPlaying();

	UFUNCTION()
	void InitializeAudioSystem();

	UFUNCTION()
	void ExitGame(UObject* Context);

	UFUNCTION()
	void PlayUISound(UObject* Context, USoundBase* UISound);

	UFUNCTION()
	void PlayHoverSound(UObject* Context);

	UFUNCTION()
	void PlayAcceptSound(UObject* Context);

	// =====================================================================
	// Campaign / OOB / Data helpers
	// =====================================================================
	void GetCampaignCombatant(int id, ECOMBATGROUP_TYPE Type);

	TArray<FS_Combatant> GetCombatantList();

	// =====================================================================
	// System Overview (3D Capture)
	// =====================================================================
	/** Ensures OverviewActor (world-owned) and OverviewRT (GI-owned) exist and are valid. */
	UFUNCTION(BlueprintCallable, Category = "System Overview")
	void EnsureSystemOverview(UObject* InWorldContext, int32 Resolution = 2048);

	/** Build the diorama bodies and capture once to OverviewRT. */
	void BuildAndCaptureSystemOverview(const TArray<struct FOverviewBody>& Bodies);

	UFUNCTION(BlueprintCallable, Category = "System Overview")
	UTextureRenderTarget2D* GetSystemOverviewRT() const { return OverviewRT; }

	UFUNCTION(BlueprintCallable, Category = "System Overview")
	ASystemOverview* GetSystemOverviewActor() const { return OverviewActor; }

	/** Optional cleanup (typically only needed on shutdown or debugging). */
	UFUNCTION(BlueprintCallable, Category = "System Overview")
	void DestroySystemOverview();

	void RebuildSystemOverview(const FS_StarMap& Star);
	void EnsureSystemOverview(UObject* Context, const FS_StarMap& StarMap, int32 Resolution);

	void LoadOrCreateUniverse();

	FString GetUniverseSlotName() const { return TEXT("Universe_Main"); }
	// =====================================================================
	// Universe Clock
	// =====================================================================
	//UFUNCTION()
	//void SetTimeScale(double NewTimeScale);

	//UFUNCTION()
	//void StartUniverseClock();

	//UFUNCTION()
	//void StopUniverseClock();

	//UFUNCTION()
	//void OnUniverseClockTick();

	//UFUNCTION()
	//void UpdateUniverseTime(float DeltaSeconds);

	//UFUNCTION()
	//void UpdatePlayerPlaytime(float DeltaSeconds);

	//UFUNCTION()
	// GetTimeScale() const { return TimeScale; }

	//int64 GetUniverseTimeSeconds() const { return UniverseTimeSeconds; }
	//int64 GetPlayerPlaytimeSeconds() const { return PlayerPlaytimeSeconds; }

	// =====================================================================
	// Universe Save API
	// =====================================================================
	void SetUniverseSaveContext(const FString& SlotName, int32 UserIndex, UUniverseSaveGame* LoadedSave);

	bool SaveUniverse();
	//bool SavePlayer(bool bForce = false);   // optional wrapper for your existing SaveGame()
	void RequestUniverseAutosave();         // optional (sets a flag)

	bool SaveCampaign();


	// =====================================================================
	// Campaign Save API
	// =====================================================================
	// Call this when a campaign is chosen OR as a safety in Start
	UCampaignSave* LoadOrCreateCampaignSave(int32 CampaignIndex, FName RowName, const FString& DisplayName);
	UCampaignSave* CreateNewCampaignSave(int32 CampaignIndex, FName RowName, const FString& DisplayName);

	// Convenience wrapper: uses current selection fields
	UCampaignSave* LoadOrCreateSelectedCampaignSave();

	// Start button can call this to guarantee CampaignSave exists before showing Operations
	void EnsureCampaignSaveLoaded();

	void SetActiveWidget(UUserWidget* Widget);
	UUserWidget* GetActiveWidget();
	
	// =====================================================================
	// Runtime Pointers
	// =====================================================================
	AGalaxy* GameGalaxy;
	AGameDataLoader* GameData;
	AAwardInfoLoader* AwardData;
	ACombatGroupLoader* CombatGroupData;

	DataLoader* loader;

	UMenuDlg* MainMenuDlg;
	UCampaignScreen* CampaignScreen;
	UOperationsScreen* OperationsScreen;
	UCampaignLoading* CampaignLoading;
	UMissionLoading* MissionLoadingScreen;
	UExitDlg* QuitDlg;
	UFirstTimeDlg* FirstTimeDlg;

	UPROPERTY()
	AMusicController* MusicController;

	AMusicController* GetMusicController();

	// =====================================================================
	// UI Audio Assets
	// =====================================================================
	UPROPERTY(EditAnywhere, Category = "UI Sound")
	USoundBase* AcceptSound;

	UPROPERTY(EditAnywhere, Category = "UI Sound")
	USoundBase* HoverSound;

	UPROPERTY(EditAnywhere, Category = "UI Sound")
	USoundBase* MenuMusic;

	// =====================================================================
	// Galaxy / Selection State
	// =====================================================================
	UPROPERTY()
	FString SelectedSystem;

	UPROPERTY()
	FString SelectedSectorName;

	UPROPERTY()
	FPlanet SelectedSector;

	UPROPERTY()
	FS_StarMap SelectedStarSystem;

	UPROPERTY(Transient)
	TObjectPtr<ASystemOverview> OverviewActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> OverviewRT = nullptr;

	UPROPERTY()
	FString LastOverviewSystemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMenuScreen> MainMenuScreenClass;

	UPROPERTY()
	TObjectPtr<UMenuScreen> MainMenuScreen = nullptr;

	UPROPERTY(Transient)
	FS_CampaignUIBundle ActiveCampaignUIBundle;

	// =====================================================================
	// Global State
	// =====================================================================
	UPROPERTY()
	double StarDate;

	UPROPERTY()
	FString PlayerSaveName;

	UPROPERTY()
	int PlayerSaveSlot;

	UPROPERTY(EditAnywhere)
	bool bClearTables;

	UPROPERTY()
	FS_PlayerGameInfo PlayerInfo;

	UPROPERTY()
	bool MissionSelectionChanged;

	UPROPERTY()
	bool ActionSelectionChanged;

	UPROPERTY()
	bool RosterSelectionChanged;

	UPROPERTY()
	EGameMode GameMode;

	// =====================================================================
	// DataTables
	// =====================================================================
	class UDataTable* CampaignDataTable;
	class UDataTable* CombatGroupDataTable;
	class UDataTable* OrderOfBattleDataTable;
	class UDataTable* CampaignOOBDataTable;
	class UDataTable* GalaxyDataTable;

	// =====================================================================
	// Data Caches
	// =====================================================================
	UPROPERTY()
	TArray<FS_Campaign> CampaignData;

	UPROPERTY()
	TArray<FS_CombatGroup> CombatRosterData;

	UPROPERTY()
	TArray<FS_CampaignMissionList> MissionList;

	UPROPERTY()
	TArray<FS_OOBForce> ForceList;

	UPROPERTY()
	TArray<FS_Galaxy> GalaxyData;

	// =====================================================================
	// Universe Time (authoritative)
	// =====================================================================
	UPROPERTY()
	FString UniverseId;

	UPROPERTY()
	uint64 UniverseSeed = 0;

	UPROPERTY()
	double UniverseTimeAccumulator = 0.0;

	// Universe save slot (provided by GameLoader once)
	FString UniverseSaveSlotName;
	int32 UniverseSaveUserIndex = 0;

	// Campaign save slot
	FString CampaignSaveSlotName;
	int32 CampaignSaveIndex = 0;

	// Save throttling
	double LastUniverseSaveRealSeconds = 0.0;
	double MinSecondsBetweenUniverseSaves = 10.0;

	int64 UniverseBaseUnixSeconds = 0;

	// Cached universe save object (optional but recommended)
	TObjectPtr<class UUniverseSaveGame> CachedUniverseSave = nullptr;

	bool bUniverseAutosaveRequested = false;
	bool bUniverseLoaded;

	UFUNCTION(BlueprintCallable, Category = "Time")
	FString GetCampaignTPlusString() const;

	UFUNCTION(BlueprintCallable, Category = "Time")
	FString GetCampaignAndUniverseTimeLine() const;




	// =====================================================================
	// Campaign Save (in-memory)
	// =====================================================================
	UPROPERTY()
	UCampaignSave* CampaignSave = nullptr;

	UPROPERTY()
	int32 SelectedCampaignIndex = 1;

	UPROPERTY()
	FName SelectedCampaignRowName = NAME_None;

	UPROPERTY()
	FString SelectedCampaignDisplayName;

	// =====================================================================
	// Active display flags
	// =====================================================================
	bool bIsDisplayUnitChanged;
	bool bIsDisplayElementChanged;

	void InitializeScreens();

protected:
	virtual void Init() override;
	virtual void Shutdown() override;

	virtual bool InitGame();



	UPROPERTY()
	FString AppName;

	UPROPERTY()
	FString TitleText;

	UPROPERTY()
	FString PaletteName;

	// Internal variables for the state of the app
	UPROPERTY()
	bool bIsWindowed;

	UPROPERTY()
	bool bIsGameActive;

	UPROPERTY()
	bool bIsDeviceLost;

	UPROPERTY()
	bool bIsMinimized;

	UPROPERTY()
	bool bIsMaximized;

	UPROPERTY()
	bool bIgnoreSizeChange;

	UPROPERTY()
	bool bIsDeviceInitialized;

	UPROPERTY()
	bool bIsDeviceRestored;

	UPROPERTY()
	FS_Campaign ActiveCampaign;

	UPROPERTY()
	int ActiveCampaignNr;

	UPROPERTY()
	bool bIsActiveCampaign;

	EGAMESTATUS Status;

	// Instance
	UPROPERTY()
	TObjectPtr<UMenuScreen> MenuScreen;

private:
	UPROPERTY()
	int64 GameTime;

	UPROPERTY()
	int64 CampaignTime;

	UPROPERTY()
	int64 GameStart;

	UPROPERTY()
	int32 SelectionMissionNr;

	UPROPERTY()
	int32 SelectionActionNr;

	UPROPERTY()
	int32 SelectionRosterNr;

	FTimerHandle TimerHandle;

	FS_OOBForce CurrentForce;
	FS_DisplayUnit DisplayUnit;
	FS_DisplayElement DisplayElement;

	UUserWidget* ActiveWidget;

	UFUNCTION()
	void OnGameTimerTick();

	void EnsureOverviewRT(int32 Resolution);
	void EnsureOverviewActor(UWorld* World);

	// Widget classes
	TSubclassOf<class UMenuScreen> MenuScreenWidgetClass;
	TSubclassOf<class UExitDlg> ExitDlgWidgetClass;
	TSubclassOf<class UFirstTimeDlg> FirstTimeDlgWidgetClass;

	void HandleUniverseMinuteAutosave(uint64 UniverseSecondsNow);

	// Timer Pub/Sub
	uint64 LastBroadcastSecond = 0;
	uint64 LastBroadcastMinute = 0;
	uint64 LastBroadcastTPlus = 0;

	void SetupMusicController();

};
