// Fill out your copyright notice in the Description page of Project Settings.


#include "SSWGameInstance.h"
#include "GameFramework/Actor.h"
#include "SimUniverse.h"
#include "Galaxy.h"
#include "DataLoader.h"
#include "Sim.h"
#include "FormattingUtils.h"

#include "MenuDlg.h"
#include "ExitDlg.h"
#include "FirstTimeDlg.h"
#include "CampaignScreen.h"
#include "OperationsScreen.h"
#include "MissionLoading.h"
#include "CampaignLoading.h"

#include "PlayerSaveGame.h"
#include "UniverseSaveGame.h" 
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h" // For data asset support

#include "MusicController.h"
#include "MusicControllerInit.h"
#include "AudioDevice.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h" 
#include "SystemOverview.h"
#include "FontManager.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterAssetRegistrySubsystem.h"

#undef UpdateResource
#undef PlaySound


USSWGameInstance::USSWGameInstance(const FObjectInitializer& ObjectInitializer) 
{
	InitializeCampaignLoadingScreen(ObjectInitializer);
	InitializeMissionBriefingScreen(ObjectInitializer);
}

void USSWGameInstance::SetProjectPath()
{
	ProjectPath = FPaths::ProjectDir();
	ProjectPath.Append(TEXT("GameData/")); 

	UE_LOG(LogTemp, Log, TEXT("Setting Game Data Directory %s"), *ProjectPath);
}

void USSWGameInstance::StartGameTimers()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(TimerHandle, this, &USSWGameInstance::OnGameTimerTick, 1.0f, true);
	}
}

FString USSWGameInstance::GetProjectPath()
{
	return ProjectPath;
}

void USSWGameInstance::Print(const FString& A, const FString& B)
{
	const FString Msg = B.IsEmpty() ? A : (A + TEXT(" ") + B);

	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, Msg);
	}
}

void USSWGameInstance::SpawnGalaxy()
{
	/*UWorld* World = GetWorld();

	FVector location = FVector::ZeroVector;
	FRotator rotate = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnInfo;
	FName Name("Starshatter Galaxy");
	SpawnInfo.Name = Name;

	if (GameGalaxy == nullptr) {
		GameGalaxy = GetWorld()->SpawnActor<AGalaxy>(AGalaxy::StaticClass(), location, rotate, SpawnInfo);


		if (GameGalaxy)
		{
			UE_LOG(LogTemp, Log, TEXT("Game Galaxy Spawned"));
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("Failed to Spawn Game Galaxy"));
		}
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Game Galaxy already exists"));
	}

	//} else {
	//	UE_LOG(LogTemp, Log, TEXT("World notxfound"));
	//}	*/	
}

void USSWGameInstance::StartGame()
{
	//SpawnUniverse();
	//SpawnGalaxy();
}

void USSWGameInstance::ShowMainMenuScreen()
{
	RemoveScreens();

	UWorld* World = GetWorld();
	if (!World) { UE_LOG(LogTemp, Error, TEXT("ShowMainMenuScreen: World is NULL")); return; }

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC) { UE_LOG(LogTemp, Error, TEXT("ShowMainMenuScreen: PC is NULL")); return; }

	if (!MenuScreenWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ShowMainMenuScreen: MenuScreenWidgetClass is NULL"));
		return;
	}

	MenuScreen = CreateWidget<UMenuScreen>(PC, MenuScreenWidgetClass);
	if (!MenuScreen) { UE_LOG(LogTemp, Error, TEXT("ShowMainMenuScreen: Failed to create MenuScreen")); return; }

	UMenuScreen* Screen = MenuScreen.Get();

	Screen->AddToViewport(100);
	Screen->SetVisibility(ESlateVisibility::Visible);

	// *** THIS IS THE MISSING PIECE ***
	Screen->Initialize(this);

	// Create dialogs AFTER Initialize resolved classes
	Screen->Setup();

	// MenuScreen decides Menu vs FirstRun
	Screen->Show();

	// Focus whatever MenuScreen decided is current
	if (UBaseScreen* Top = Screen->GetCurrentDialog())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Top->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowMainMenuScreen: No CurrentDialog after Show()"));
	}
}

void USSWGameInstance::LoadTransitionScreen()
{
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
			PlayerController->bShowMouseCursor = false; UGameplayStatics::OpenLevel(this, "Transition");
		}
	}
}

void USSWGameInstance::LoadMissionBriefingScreen()
{
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
			PlayerController->bShowMouseCursor = false; UGameplayStatics::OpenLevel(this, "MissionBriefing");
		}
	}
}

void USSWGameInstance::LoadGameLevel(FString LevelName)
{
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameAndUI InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
			PlayerController->bShowMouseCursor = false; UGameplayStatics::OpenLevel(this, FName(LevelName));
		}
	}
}

void USSWGameInstance::Init()
{
	Super::Init();
	bIsWindowed = false;
	bIsGameActive = false;
	bIsDeviceLost = false;
	bIsMinimized = false;
	bIsMaximized = false;
	bIgnoreSizeChange = false;
	bIsDeviceInitialized = false;
	bIsDeviceRestored = false;

	PlayerSaveName = "PlayerSaveSlot";
	PlayerSaveSlot = 0;
	UniverseSaveSlotName = "Universe_Main";
	UniverseSaveUserIndex = 0;
	CampaignSaveSlotName = "Campaign";
	CampaignSaveIndex = 0;

	FDateTime GameDate(2228, 1, 1);
	SetGameTime(GameDate.ToUnixTimestamp());
	SetProjectPath();

	CampaignData.SetNum(5); // number of campaigns

	loader = DataLoader::GetLoader();

	Status = EGAMESTATUS::OK;
	UE_LOG(LogTemp, Log, TEXT("Initializing Game\n."));

	if (Status == EGAMESTATUS::OK) {
		UE_LOG(LogTemp, Log, TEXT("\n  Initializing instance...\n"));
	}

	if (Status == EGAMESTATUS::OK) {
		UE_LOG(LogTemp, Log, TEXT("  Initializing content...\n"));
		InitContent();
	}

	/*if (UGameplayStatics::DoesSaveGameExist(PlayerSaveName, PlayerSaveSlot)) {
		LoadGame(PlayerSaveName, PlayerSaveSlot);
		UE_LOG(LogTemp, Log, TEXT("Player Name: %s"), *PlayerInfo.Name);

		if (PlayerInfo.Campaign >= 0) {
			ReadCampaignData();
		}
	}*/

	SetupMusicController();
	//ExportDataTableToCSV(OrderOfBattleDataTable, TEXT("OOBExport.csv"));
	if (UTimerSubsystem* Timer = GetSubsystem<UTimerSubsystem>())
	{
		Timer->OnUniverseMinute.AddUObject(this, &USSWGameInstance::HandleUniverseMinuteAutosave);
		// optional: OnUniverseSecond for finer cadence, but minute is safer.
	}

	FontManager::RegisterAllFonts(this);
}

void USSWGameInstance::SetActiveUnit(bool bShow, FString Name, EEMPIRE_NAME Empire, ECOMBATGROUP_TYPE Type, FString Loc)
{
	DisplayUnit.bShowUnit = bShow;
	DisplayUnit.Name = Name;
	DisplayUnit.Empire = Empire;
	DisplayUnit.Type = Type;
	DisplayUnit.Location = Loc;
}

void USSWGameInstance::SetActiveElement(bool bShow, FString Name, EEMPIRE_NAME Empire, ECOMBATUNIT_TYPE Type, FString Loc)
{
	DisplayElement.bShowUnit = bShow;
	DisplayElement.Name = Name;
	DisplayElement.Empire = Empire;
	DisplayElement.Type = Type;
	DisplayElement.Location = Loc;
}

void USSWGameInstance::SetActiveWidget(UUserWidget* Widget)
{
	ActiveWidget = Widget;
}

UUserWidget* USSWGameInstance::GetActiveWidget() {
	return ActiveWidget;
}

FS_DisplayUnit USSWGameInstance::GetActiveUnit()
{
	return DisplayUnit;
}

FS_DisplayElement USSWGameInstance::GetActiveElement()
{
	return DisplayElement;
}

FS_OOBForce USSWGameInstance::GetActiveOOBForce() {
	return CurrentForce;
}

void USSWGameInstance::Shutdown()
{
	Super::Shutdown();

	if (FAudioDevice* AudioDevice = GEngine->GetMainAudioDeviceRaw())
	{
		AudioDevice->Flush(nullptr); // Stops all active sounds immediately
	}
}

bool USSWGameInstance::InitContent()
{
	List<Text>  bundles;
	
	FString ContentProjectPath = FPaths::ProjectDir();
	ProjectPath.Append(TEXT("GameData/Content/"));

	loader->SetDataPath(TCHAR_TO_ANSI(*ContentProjectPath));
	//loader->ListFiles("content*", bundles);

	FString GameDataProjectPath = FPaths::ProjectDir(); 
	GameDataProjectPath = FPaths::ProjectDir();
	GameDataProjectPath.Append(TEXT("GameData/"));
	loader->SetDataPath(TCHAR_TO_ANSI(*GameDataProjectPath));

	return true;
}

bool USSWGameInstance::InitGame()
{
	return false;
}

void USSWGameInstance::InitializeScreens()
{
	UStarshatterAssetRegistrySubsystem* Assets =
		GetSubsystem<UStarshatterAssetRegistrySubsystem>();

	if (!ensure(Assets))
	{
		return;
	}

	// MenuScreen
	MenuScreenWidgetClass = Assets->GetWidgetClass(TEXT("UI.MenuScreenClass"), true);
	if (!ensureMsgf(MenuScreenWidgetClass,
		TEXT("[UI] UI.MenuScreenClass not bound or not a widget class")))
	{
		return;
	}

	// FirstRunDlg
	FirstTimeDlgWidgetClass = Assets->GetWidgetClass(TEXT("UI.FirstTimeDlgClass"), true);
	if (!ensureMsgf(FirstTimeDlgWidgetClass,
		TEXT("[UI] UI.FirstTimeDlgClass not bound or not a widget class")))
	{
		return;
	}

	// Exit/Quit dialog
	ExitDlgWidgetClass = Assets->GetWidgetClass(TEXT("UI.ExitDlgClass"), /*bLoadNow=*/true);

	if (!ExitDlgWidgetClass)
	{
		// This is almost always because Project Settings is bound to WB_QuitDlg (WidgetBlueprint)
		// instead of WB_QuitDlg_C (GeneratedClass).
		UE_LOG(LogTemp, Error,
			TEXT("[UI] UI.ExitDlgClass invalid. Bind the GENERATED CLASS (…_C) in Project Settings. "
				"Example: /Game/Screens/WB_QuitDlg.WB_QuitDlg_C"));

		return;
	}
}

void USSWGameInstance::InitializeMissionBriefingScreen(const FObjectInitializer& ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UMissionLoading> MissionLoadingScreenWidget(TEXT("/Game/Screens/Mission/WB_MissionLoading"));
	if (!ensure(MissionLoadingScreenWidget.Class != nullptr))
	{
		return;
	}
	MissionLoadingWidgetClass = MissionLoadingScreenWidget.Class;
}

void USSWGameInstance::InitializeCampaignLoadingScreen(const FObjectInitializer& ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UCampaignLoading> CampaignLoadingWidget(TEXT("/Game/Screens/Campaign/WB_CampaignLoading"));
	if (!ensure(CampaignLoadingWidget.Class != nullptr))
	{
		return;
	}
	CampaignLoadingWidgetClass = CampaignLoadingWidget.Class;
}

void USSWGameInstance::RemoveScreens()
{
	if (MenuScreen)
	{
		MenuScreen->TearDown();
		MenuScreen->RemoveFromParent();
		MenuScreen = nullptr;
	}
	
	if (CampaignScreen) {
		//RemoveCampaignScreen();
	}
	if (CampaignLoading) {
		//RemoveCampaignLoadScreen();
	}
	if (OperationsScreen) {
		//RemoveOperationsScreen();
	}
	if (MainMenuDlg) {
		//RemoveMainMenuScreen();
	}
	if (MissionLoadingScreen) {
		//RemoveMissionBriefingScreen();
	}
}

void USSWGameInstance::OnGameTimerTick()
{
	SetGameTime(GetGameTime() + 1);
	SetCampaignTime(GetCampaignTime() + 1);
	UE_LOG(LogTemp, Log, TEXT("Campaign Timer: %d"), GetCampaignTime());
}

void USSWGameInstance::ShowCampaignLoading()
{
	//RemoveScreens();

	// Create widget
	//if (!CampaignLoading) {
	CampaignLoading = CreateWidget<UCampaignLoading>(this, CampaignLoadingWidgetClass);
	//}
	// Add it to viewport
	CampaignLoading->AddToViewport(102);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(CampaignLoading->TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	ToggleCampaignLoading(true);
}

void USSWGameInstance::ShowMissionBriefingScreen()
{
	//RemoveScreens();

	// Create widget
	//if (!MissionLoadingScreen) {
		// Create widget
	MissionLoadingScreen = CreateWidget<UMissionLoading>(this, MissionLoadingWidgetClass);
	//}

	// Add it to viewport
	MissionLoadingScreen->AddToViewport(101);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(MissionLoadingScreen->TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	ToggleMissionBriefingScreen(true);
}

void USSWGameInstance::ToggleMissionBriefingScreen(bool bVisible)
{
	if (MissionLoadingScreen) {
		if (bVisible) {
			MissionLoadingScreen->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			MissionLoadingScreen->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USSWGameInstance::RemoveMainMenuScreen()
{
	if (MainMenuDlg) {
		MainMenuDlg->RemoveFromParent();

		MainMenuDlg = nullptr;
		if (GEngine) {
			GEngine->ForceGarbageCollection();
		}
	}
}

void USSWGameInstance::RemoveCampaignLoadScreen()
{
	if (CampaignLoading) {
		CampaignLoading->RemoveFromParent();

		CampaignLoading = nullptr;
		if (GEngine) {
			GEngine->ForceGarbageCollection();
		}
	}
}

void USSWGameInstance::RemoveMissionBriefingScreen()
{
	if (MissionLoadingScreen) {
		MissionLoadingScreen->RemoveFromParent();

		MissionLoadingScreen = nullptr;
		if (GEngine) {
			GEngine->ForceGarbageCollection();
		}
	}
}

void USSWGameInstance::ToggleCampaignLoading(bool bVisible)
{
	if (CampaignLoading) {
		if (bVisible) {
			CampaignLoading->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			CampaignLoading->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USSWGameInstance::SetGameMode(EGameMode gm)
{
	GameMode = gm;
}

EGameMode USSWGameInstance::GetGameMode()
{
	return GameMode;
}

void USSWGameInstance::SetActiveCampaign(FS_Campaign campaign)
{
	ActiveCampaign = campaign;
}

void USSWGameInstance::SetActiveCampaignNr(int active)
{
	ActiveCampaignNr = active;
}

void USSWGameInstance::SetSelectedMissionNr(int active)
{
	SelectionMissionNr = active;
}

void USSWGameInstance::SetSelectedActionNr(int active)
{
	SelectionActionNr = active;
}

void USSWGameInstance::SetSelectedRosterNr(int active)
{
	SelectionRosterNr = active;
}

void USSWGameInstance::SetCampaignActive(bool bIsActive)
{
	bIsActiveCampaign = bIsActive;
}

FS_Campaign USSWGameInstance::GetActiveCampaign()
{
	return ActiveCampaign;
}

int USSWGameInstance::GetActiveCampaignNr()
{
	return ActiveCampaignNr;
}

int USSWGameInstance::GetSelectedMissionNr()
{
	return SelectionMissionNr;
}

int USSWGameInstance::GetSelectedActionNr()
{
	return SelectionActionNr;
}

int USSWGameInstance::GetSelectedRosterNr()
{
	return SelectionRosterNr;
}

bool USSWGameInstance::GetCampaignActive()
{
	return bIsActiveCampaign;
}

void USSWGameInstance::SetGameTime(int64 time)
{
	GameTime = time;
}

int64 USSWGameInstance::GetGameTime()
{
	return GameTime;
}

void USSWGameInstance::SetCampaignTime(int64 time)
{
	CampaignTime = time;
}

int64 USSWGameInstance::GetCampaignTime()
{
	return CampaignTime;
}


UTexture2D* USSWGameInstance::LoadPNGTextureFromFile(const FString& Path)
{
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *Path)) {
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *Path);
		return nullptr;
	}

	// Get image wrapper for PNG
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	// Decode PNG
	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num())) {
		TArray<uint8> RawData;
		
		if (ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, RawData)) {
			int32 Width = ImageWrapper->GetWidth();
			int32 Height = ImageWrapper->GetHeight();

			// Create the texture
			UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
			if (!Texture) return nullptr;

			// Lock and fill mip data
			void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
			Texture->GetPlatformData()->Mips[0].BulkData.Unlock();

			// Update texture
			Texture->UpdateResource();
			return Texture;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Failed to decode PNG: %s"), *FilePath);
	return nullptr;
}

void USSWGameInstance::PlayMusic(USoundBase* Music)
{
	if (!Music)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayMusic: Invalid SoundBase"));
		return;
	}
	
	if (MusicController)
	{
		MusicController->PlayMusic(Music);
	}
}

void USSWGameInstance::StopMusic()
{
	if (MusicController) {
		MusicController->StopMusic();
	}
}

void USSWGameInstance::PlayMenuMusic()
{
	PlayMusic(MenuMusic);
}

void USSWGameInstance::PlaySoundFromFile(FString& AudioPath)
{
	USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *AudioPath));

	if (!Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayUISound: Invalid SoundBase"));
		return;
	}

	if (AMusicController* Music = GetMusicController())
	{
		Music->PlayUISound(Sound);
	}
}

bool USSWGameInstance::IsSoundPlaying()
{
	if (AMusicController* MC = GetMusicController())
	{
		return MC->IsSoundPlaying();
	}
	return false;
}

void USSWGameInstance::InitializeAudioSystem()
{
	UWorld* World = GetWorld();
	if (World)
	{
		MusicController = World->SpawnActor<AMusicController>();
		UE_LOG(LogTemp, Log, TEXT("Music Controller Spawned"));
	}
}

void USSWGameInstance::ExitGame(UObject* Context) {
	UKismetSystemLibrary::QuitGame(Context, 0, EQuitPreference::Quit, true);
}

void USSWGameInstance::PlayUISound(UObject* Context, USoundBase* UISound)
{
	if (UISound)
	{
		UGameplayStatics::PlaySound2D(Context, UISound);
	}
}

void USSWGameInstance::PlayHoverSound(UObject* Context)
{
	PlayUISound(Context, HoverSound);
}

void USSWGameInstance::PlayAcceptSound(UObject* Context)
{
	PlayUISound(Context, AcceptSound);
}


void USSWGameInstance::GetCampaignCombatant(int id, ECOMBATGROUP_TYPE Type) {
// Filters Table by Active in campaign
}

TArray<FS_Combatant> USSWGameInstance::GetCombatantList()
{
	return CampaignData[PlayerInfo.Campaign].Combatant;
}

void USSWGameInstance::FlattenForce(const FS_OOBForce& Force, TArray<FS_OOBFlatEntry>& OutFlatList)
{
	int32 CurrentId = 0;
	RecursivelyFlattenForce(Force, INDEX_NONE, 0, CurrentId, OutFlatList);
}

void USSWGameInstance::RecursivelyFlattenForce(
	const FS_OOBForce& Force,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry ForceEntry;
	ForceEntry.Id = ThisId;
	ForceEntry.ParentId = ParentId;
	ForceEntry.DisplayName = Force.Name;
	ForceEntry.IndentLevel = IndentLevel;
	ForceEntry.GroupType = ECOMBATGROUP_TYPE::FORCE;

	OutFlatList.Add(ForceEntry);

	for (const FS_OOBFleet& Fleet : Force.Fleet)
	{
		RecursivelyFlattenFleet(Fleet, ThisId, IndentLevel + 1, CurrentId, OutFlatList);
	}
}

void USSWGameInstance::RecursivelyFlattenFleet(
	const FS_OOBFleet& Fleet,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry FleetEntry;
	FleetEntry.Id = ThisId;
	FleetEntry.ParentId = ParentId;
	FleetEntry.DisplayName = Fleet.Name;
	FleetEntry.IndentLevel = IndentLevel;
	FleetEntry.GroupType = ECOMBATGROUP_TYPE::FLEET;

	OutFlatList.Add(FleetEntry);

	for (const FS_OOBCarrier& Carrier : Fleet.Carrier)
	{
		RecursivelyFlattenCarrier(Carrier, ThisId, IndentLevel + 1, CurrentId, OutFlatList);
	}

	for (const FS_OOBDestroyer& Destroyer : Fleet.Destroyer)
	{
		RecursivelyFlattenDestroyer(Destroyer, ThisId, IndentLevel + 1, CurrentId, OutFlatList);
	}
}

void USSWGameInstance::RecursivelyFlattenCarrier(
	const FS_OOBCarrier& Carrier,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry Entry;
	Entry.Id = ThisId;
	Entry.ParentId = ParentId;
	Entry.DisplayName = Carrier.Name;
	Entry.IndentLevel = IndentLevel;
	Entry.GroupType = ECOMBATGROUP_TYPE::CARRIER_GROUP;

	OutFlatList.Add(Entry);

	for (const FS_OOBWing& Wing : Carrier.Wing)
	{
		RecursivelyFlattenWing(Wing, ThisId, IndentLevel + 1, CurrentId, OutFlatList);
	}
}

void USSWGameInstance::RecursivelyFlattenDestroyer(
	const FS_OOBDestroyer& Destroyer,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry Entry;
	Entry.Id = ThisId;
	Entry.ParentId = ParentId;
	Entry.DisplayName = Destroyer.Name;
	Entry.IndentLevel = IndentLevel;
	Entry.GroupType = ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;

	OutFlatList.Add(Entry);
}

void USSWGameInstance::RecursivelyFlattenWing(
	const FS_OOBWing& Wing,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry Entry;
	Entry.Id = ThisId;
	Entry.ParentId = ParentId;
	Entry.DisplayName = Wing.Name;
	Entry.IndentLevel = IndentLevel;
	Entry.GroupType = ECOMBATGROUP_TYPE::WING;

	OutFlatList.Add(Entry);

	for (const FS_OOBFighter& Unit : Wing.Fighter)
	{
		RecursivelyFlattenFighter(Unit, ThisId, IndentLevel + 1, CurrentId, OutFlatList);
	}

	for (const FS_OOBAttack& Attack : Wing.Attack)
	{
		RecursivelyFlattenAttack(Attack, ThisId, IndentLevel + 1, CurrentId, OutFlatList);
	}

	for (const FS_OOBIntercept& Intercept : Wing.Intercept)
	{
		RecursivelyFlattenIntercept(Intercept, ThisId, IndentLevel + 1, CurrentId, OutFlatList);
	}

	for (const FS_OOBLanding& Landing : Wing.Landing)
	{
		RecursivelyFlattenLanding(Landing, ThisId, IndentLevel + 1, CurrentId, OutFlatList);
	}
}

void USSWGameInstance::RecursivelyFlattenFighter(
	const FS_OOBFighter& Unit,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry Entry;
	Entry.Id = ThisId;
	Entry.ParentId = ParentId;
	Entry.DisplayName = Unit.Name;
	Entry.IndentLevel = IndentLevel;
	Entry.GroupType = ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;

	OutFlatList.Add(Entry);
}
void USSWGameInstance::RecursivelyFlattenAttack(
	const FS_OOBAttack& Attack,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry Entry;
	Entry.Id = ThisId;
	Entry.ParentId = ParentId;
	Entry.DisplayName = Attack.Name;
	Entry.IndentLevel = IndentLevel;
	Entry.GroupType = ECOMBATGROUP_TYPE::ATTACK_SQUADRON;

	OutFlatList.Add(Entry);
}

void USSWGameInstance::RecursivelyFlattenIntercept(
	const FS_OOBIntercept& Intercept,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry Entry;
	Entry.Id = ThisId;
	Entry.ParentId = ParentId;
	Entry.DisplayName = Intercept.Name;
	Entry.IndentLevel = IndentLevel;
	Entry.GroupType = ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;

	OutFlatList.Add(Entry);
}

void USSWGameInstance::RecursivelyFlattenLanding(
	const FS_OOBLanding& Landing,
	int32 ParentId,
	int32 IndentLevel,
	int32& CurrentId,
	TArray<FS_OOBFlatEntry>& OutFlatList
)
{
	int32 ThisId = CurrentId++;

	FS_OOBFlatEntry Entry;
	Entry.Id = ThisId;
	Entry.ParentId = ParentId;
	Entry.DisplayName = Landing.Name;
	Entry.IndentLevel = IndentLevel;
	Entry.GroupType = ECOMBATGROUP_TYPE::LCA_SQUADRON;

	OutFlatList.Add(Entry);
}

void USSWGameInstance::SetupMusicController()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<AMusicController> It(World); It; ++It)
	{
		return; // already exists
	}

	FVector Location = FVector(-1000, 0, 0);
	World->SpawnActor<AMusicController>(AMusicController::StaticClass(), Location, FRotator::ZeroRotator);
}

void USSWGameInstance::EnsureSystemOverview(UObject* InWorldContext, int32 Resolution)
{
	if (!InWorldContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnsureSystemOverview: InWorldContext is null"));
		return;
	}

	UWorld* World = InWorldContext->GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnsureSystemOverview: World is null"));
		return;
	}

	EnsureOverviewRT(Resolution);
	EnsureOverviewActor(World);

	if (OverviewActor && OverviewRT)
	{
		OverviewActor->SetRenderTarget(OverviewRT);
		OverviewActor->ConfigureCaptureForUI();
	}
}

void USSWGameInstance::EnsureOverviewRT(int32 Resolution)
{
	Resolution = FMath::Clamp(Resolution, 256, 4096);

	// If RT exists but wrong size, recreate it (optional).
	if (OverviewRT && OverviewRT.Get())
	{
		if (OverviewRT->SizeX == Resolution && OverviewRT->SizeY == Resolution)
		{
			return;
		}

		// Recreate to new size
		OverviewRT = nullptr;
	}

	// IMPORTANT: Outer = GameInstance, so it survives widget rebuilds and UI transitions
	OverviewRT = NewObject<UTextureRenderTarget2D>(this, TEXT("RT_SystemOverview"), RF_Transient);
	if (!OverviewRT)
	{
		UE_LOG(LogTemp, Error, TEXT("EnsureOverviewRT: Failed to allocate OverviewRT"));
		return;
	}

	OverviewRT->RenderTargetFormat = RTF_RGBA16f;
	OverviewRT->ClearColor = FLinearColor::Black;
	OverviewRT->bAutoGenerateMips = false;
	OverviewRT->InitAutoFormat(Resolution, Resolution);
	OverviewRT->UpdateResourceImmediate(true);

	UE_LOG(LogTemp, Log, TEXT("EnsureOverviewRT: Created OverviewRT %s (%dx%d) Outer=%s"),
		*GetNameSafe(OverviewRT.Get()), OverviewRT->SizeX, OverviewRT->SizeY, *GetNameSafe(this));
}

void USSWGameInstance::EnsureOverviewActor(UWorld* World)
{
	if (!World)
	{
		return;
	}

	// Actor is world-owned. GI holds a reference and respawns if level/world changes.
	if (OverviewActor && IsValid(OverviewActor))
	{
		if (OverviewActor->GetWorld() == World)
		{
			return;
		}

		// Different world (level travel): drop pointer so we respawn
		OverviewActor = nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Name = TEXT("SystemOverviewActor");

	// Spawn far away so it's never seen in the playable scene
	const FVector SpawnLoc(1000000.f, 1000000.f, 1000000.f);
	const FRotator SpawnRot = FRotator::ZeroRotator;

	OverviewActor = World->SpawnActor<ASystemOverview>(ASystemOverview::StaticClass(), SpawnLoc, SpawnRot, Params);
	if (!OverviewActor)
	{
		UE_LOG(LogTemp, Error, TEXT("EnsureOverviewActor: Failed to spawn ASystemOverview"));
		return;
	}

	OverviewActor->SetActorHiddenInGame(true);
	OverviewActor->SetActorEnableCollision(false);

	UE_LOG(LogTemp, Log, TEXT("EnsureOverviewActor: Spawned %s in World=%s"),
		*GetNameSafe(OverviewActor), *GetNameSafe(World));
}

void USSWGameInstance::BuildAndCaptureSystemOverview(const TArray<FOverviewBody>& Bodies)
{
	if (!OverviewActor || !IsValid(OverviewActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildAndCaptureSystemOverview: OverviewActor invalid"));
		return;
	}

	if (!OverviewRT || !IsValid(OverviewRT))
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildAndCaptureSystemOverview: OverviewRT invalid"));
		return;
	}

	OverviewActor->SetRenderTarget(OverviewRT);
	OverviewActor->BuildDiorama(Bodies);
	OverviewActor->CaptureOnce();
}

void USSWGameInstance::DestroySystemOverview()
{
	if (OverviewActor && IsValid(OverviewActor))
	{
		OverviewActor->Destroy();
	}
	OverviewActor = nullptr;

	// Usually keep the RT alive, since you said GI should own it.
	// If you want to fully clean up, uncomment:
	// OverviewRT = nullptr;
}

void USSWGameInstance::RebuildSystemOverview(const FS_StarMap& StarMap)
{
	if (!OverviewActor)
	{
		return;
	}

	TArray<FOverviewBody> Bodies;
	Bodies.Reserve(1 + StarMap.Planet.Num() * 2); // rough reserve

	// Star (index 0)
	FOverviewBody Star;
	Star.Name = StarMap.Name;
	Star.ParentIndex = INDEX_NONE;
	Star.RadiusKm = StarMap.Radius;
	Star.OrbitKm = 0.f;
	Bodies.Add(Star);

	const int32 StarIndex = 0;

	// Planets
	for (const FS_PlanetMap& Planet : StarMap.Planet)
	{
		FOverviewBody PlanetBody;
		PlanetBody.Name = Planet.Name;
		PlanetBody.ParentIndex = StarIndex;
		PlanetBody.OrbitKm = Planet.Orbit;
		PlanetBody.RadiusKm = Planet.Radius;
		PlanetBody.OrbitAngleDeg = Planet.OrbitAngle;      // if you have it; otherwise random/cache
		PlanetBody.InclinationDeg = Planet.Inclination;    // if present
		const int32 ThisPlanetIndex = Bodies.Add(PlanetBody);

		// Moons (parent = this planet)
		for (const FS_MoonMap& Moon : Planet.Moon)
		{
			FOverviewBody MoonBody;
			MoonBody.Name = Moon.Name;
			MoonBody.ParentIndex = ThisPlanetIndex;
			MoonBody.OrbitKm = Moon.Orbit;
			MoonBody.RadiusKm = Moon.Radius;
			MoonBody.InclinationDeg = Moon.Inclination;
			MoonBody.OrbitAngleDeg = Moon.OrbitAngle;       // if you have it; otherwise random/cache
			Bodies.Add(MoonBody);
		}
	}

	OverviewActor->BuildDiorama(Bodies);
	OverviewActor->FrameDiorama();
	OverviewActor->CaptureOnce();
}

void USSWGameInstance::EnsureSystemOverview(
	UObject* Context,
	const FS_StarMap& StarMap,
	int32 Resolution)
{
	if (!Context) return;

	UWorld* World = Context->GetWorld();
	if (!World) return;

	EnsureOverviewRT(Resolution);
	EnsureOverviewActor(World);

	if (!OverviewActor || !OverviewRT) return;

	OverviewActor->SetRenderTarget(OverviewRT);
	OverviewActor->ConfigureCaptureForUI();

	// Only rebuild if system changed
	if (LastOverviewSystemName != StarMap.Name)
	{
		LastOverviewSystemName = StarMap.Name;
		RebuildSystemOverview(StarMap);
	}
}

//void USSWGameInstance::SetTimeScale(double NewTimeScale)
//{
//	TimeScale = FMath::Clamp(NewTimeScale, 0.0, 1.0e7);
//	UE_LOG(LogTemp, Warning, TEXT("TimeScale set to %.2f"), TimeScale);
//}

/*void USSWGameInstance::StartUniverseClock()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartUniverseClock: World is null"));
		return;
	}

	if (TimeStepSeconds <= 0.0)
	{
		TimeStepSeconds = 1.0;
	}

	FTimerManager& TM = World->GetTimerManager();

	// Prevent duplicates
	if (TM.IsTimerActive(UniverseTimerHandle))
	{
		TM.ClearTimer(UniverseTimerHandle);
	}

	TM.SetTimer(
		UniverseTimerHandle,
		this,
		&USSWGameInstance::OnUniverseClockTick,
		(float)TimeStepSeconds,
		true
	);

	UE_LOG(LogTemp, Log, TEXT("Universe clock started: Active=%s Step=%.3f TimeScale=%.2f"),
		TM.IsTimerActive(UniverseTimerHandle) ? TEXT("TRUE") : TEXT("FALSE"),
		TimeStepSeconds,
		TimeScale);
}

void USSWGameInstance::StopUniverseClock()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UniverseTimerHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("Universe clock stopped"));
}

void USSWGameInstance::OnUniverseClockTick()
{
	// Advance universe time
	const double DeltaUniverse = TimeStepSeconds * TimeScale;
	const uint64 AddSeconds = (uint64)FMath::Max(1.0, FMath::RoundToDouble(DeltaUniverse));
	UniverseTimeSeconds += AddSeconds;

	// ---------------------------------------------
	// PUB-SUB BROADCASTS
	// ---------------------------------------------

	// 1) Per-second broadcast
	if (UniverseTimeSeconds != LastBroadcastSecond)
	{
		LastBroadcastSecond = UniverseTimeSeconds;
		OnUniverseSecond.Broadcast(UniverseTimeSeconds);
	}

	// 2) Per-minute broadcast (Intel refresh)
	const uint64 CurrentMinute = UniverseTimeSeconds / 60ULL;
	if (CurrentMinute != LastBroadcastMinute)
	{
		LastBroadcastMinute = CurrentMinute;
		OnUniverseMinute.Broadcast(UniverseTimeSeconds);
	}

	// 3) Campaign T+ broadcast (only if campaign active)
	if (CampaignSave)
	{
		const uint64 TPlus = CampaignSave->GetTPlusSeconds(UniverseTimeSeconds);
		if (TPlus != LastBroadcastTPlus)
		{
			LastBroadcastTPlus = TPlus;
			OnCampaignTPlusChanged.Broadcast(UniverseTimeSeconds, TPlus);
		}
	}*/

	// ---------------------------------------------
	// Autosave / playtime logic (unchanged)
	// ---------------------------------------------
	//PlayerPlaytimeSeconds += (int64)TimeStepSeconds;

	//if (bUniverseAutosaveRequested)
	//{
	//	SaveUniverse();
		//bUniverseAutosaveRequested = false;
	//}
//}

/*void USSWGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// Timers belong to the loaded UWorld, so restart the clock after travel
	StartUniverseClock();

	UE_LOG(LogTemp, Log, TEXT("HandlePostLoadMap: restarted universe clock for World=%s"),
		*GetNameSafe(LoadedWorld));
}*/

void USSWGameInstance::SetUniverseSaveContext(const FString& SlotName, int32 UserIndex, UUniverseSaveGame* LoadedSave)
{
	UniverseSaveSlotName = SlotName;
	UniverseSaveUserIndex = UserIndex;
	CachedUniverseSave = LoadedSave;

	UE_LOG(LogTemp, Log, TEXT("Universe save context set: Slot=%s UserIndex=%d SaveObj=%s"),
		*UniverseSaveSlotName, UniverseSaveUserIndex, *GetNameSafe(CachedUniverseSave.Get()));
}

bool USSWGameInstance::SaveUniverse()
{
	const UTimerSubsystem* Timer = UGameInstance::GetSubsystem<UTimerSubsystem>();
	UE_LOG(LogTemp, Error, TEXT("in USSWGameInstance::SaveUniverse()"));

	// Guard: must have valid save context
	if (UniverseId.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("SaveUniverse: UniverseId is empty (notxloaded?)"));
		return false;
	}

	// Create a NEW object each time (no caching, no GC surprises)
	UUniverseSaveGame* SaveObj = Cast<UUniverseSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UUniverseSaveGame::StaticClass())
	);
	if (!SaveObj)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveUniverse: CreateSaveGameObject failed"));
		return false;
	}

	SaveObj->UniverseId = UniverseId;
	SaveObj->UniverseSeed = UniverseSeed;
	SaveObj->UniverseBaseUnixSeconds = Timer->UniverseBaseUnixSeconds;
	SaveObj->UniverseTimeSeconds = Timer->UniverseTimeSeconds;

	// Use slot saving first (simplest + safest)
	const FString Slot = GetUniverseSlotName();   // MUST return a valid slot
	constexpr int32 UserIndex = 0;

	const bool bOK = UGameplayStatics::SaveGameToSlot(SaveObj, Slot, UserIndex);
	UE_LOG(LogTemp, Warning, TEXT("SaveUniverse: slot=%s ok=%d time=%llu"),
		*Slot, bOK ? 1 : 0, (unsigned long long)Timer->UniverseTimeSeconds);

	return bOK;
}

void USSWGameInstance::RequestUniverseAutosave()
{
	bUniverseAutosaveRequested = true;
}

#include "CampaignSave.h"
#include "Kismet/GameplayStatics.h"

bool USSWGameInstance::SaveCampaign()
{
	if (!CampaignSave)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveCampaign: No CampaignSave loaded"));
		return false;
	}

	if (CampaignSave->CampaignRowName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("SaveCampaign: CampaignRowName is None"));
		return false;
	}

	const FString Slot =
		UCampaignSave::MakeSlotNameFromRowName(CampaignSave->CampaignRowName);

	constexpr int32 UserIndex = 0;

	const bool bOK =
		UGameplayStatics::SaveGameToSlot(CampaignSave, Slot, UserIndex);

	UE_LOG(LogTemp, Warning,
		TEXT("SaveCampaign: slot=%s ok=%d row=%s"),
		*Slot,
		bOK ? 1 : 0,
		*CampaignSave->CampaignRowName.ToString());

	return bOK;
}

UCampaignSave* USSWGameInstance::LoadOrCreateCampaignSave(int32 CampaignIndex, FName RowName, const FString& DisplayName)
{
	// Normalize
	CampaignIndex = FMath::Max(1, CampaignIndex);

	const FString Slot = UCampaignSave::MakeSlotNameFromRowName(RowName);
	constexpr int32 UserIndex = 0;

	UTimerSubsystem* Timer = GetSubsystem<UTimerSubsystem>();

	// Try load
	if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Slot, UserIndex))
	{
		if (UCampaignSave* LoadedSave = Cast<UCampaignSave>(Loaded))
		{
			UE_LOG(LogTemp, Warning, TEXT("Campaign load attempt: Index=%d Slot=%s"),
				CampaignIndex, *Slot);

			UE_LOG(LogTemp, Warning, TEXT("Campaign load result: %s"),
				Loaded ? TEXT("SUCCESS") : TEXT("FAIL"));

			// Repair identity fields (optional)
			if (LoadedSave->CampaignIndex != CampaignIndex)
				LoadedSave->CampaignIndex = CampaignIndex;

			if (LoadedSave->CampaignRowName.IsNone() && !RowName.IsNone())
				LoadedSave->CampaignRowName = RowName;

			if (LoadedSave->CampaignDisplayName.IsEmpty() && !DisplayName.IsEmpty())
				LoadedSave->CampaignDisplayName = DisplayName;

			CampaignSave = LoadedSave;

			if (Timer)
			{
				// ---- One-time repair for older saves that never stored the anchor ----
				if (!CampaignSave->bInitialized || CampaignSave->CampaignStartUniverseSeconds == 0)
				{
					const uint64 Now = Timer->GetUniverseTimeSeconds();
					CampaignSave->InitializeCampaignClock(Now);

					// Persist the repaired anchor so it doesn't "reset" next load:
					UGameplayStatics::SaveGameToSlot(CampaignSave, Slot, UserIndex);
				}

				CampaignSave = LoadedSave;
				UE_LOG(LogTemp, Warning,
					TEXT("Loaded campaign from slot=%s  ObjName=%s  Row=%s  Index=%d  Start=%llu  Init=%d"),
					*Slot,
					*GetNameSafe(LoadedSave),
					*LoadedSave->CampaignRowName.ToString(),
					LoadedSave->CampaignIndex,
					(unsigned long long)LoadedSave->CampaignStartUniverseSeconds,
					LoadedSave->bInitialized ? 1 : 0);

				Timer->SetCampaignSave(LoadedSave);  // explicitly LoadedSave
			}

			return CampaignSave;
		}
	}

	// Create new
	UCampaignSave* NewSave = Cast<UCampaignSave>(
		UGameplayStatics::CreateSaveGameObject(UCampaignSave::StaticClass())
	);
	if (!NewSave)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadOrCreateCampaignSave: Failed to create SaveGame object"));
		return nullptr;
	}

	NewSave->CampaignIndex = CampaignIndex;
	NewSave->CampaignRowName = RowName;
	NewSave->CampaignDisplayName = DisplayName;

	// Anchor campaign clock to CURRENT universe time (from subsystem)
	const uint64 NowUniverse = Timer ? Timer->GetUniverseTimeSeconds() : 0ULL;
	NewSave->InitializeCampaignClock(NowUniverse);

	// Persist
	if (!UGameplayStatics::SaveGameToSlot(NewSave, Slot, UserIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("LoadOrCreateCampaignSave: SaveGameToSlot failed for %s"), *Slot);
	}

	// Assign + inject
	CampaignSave = NewSave;

	if (Timer)
	{
		Timer->SetCampaignSave(CampaignSave);
	}

	return CampaignSave;
}

UCampaignSave* USSWGameInstance::CreateNewCampaignSave(int32 CampaignIndex, FName RowName, const FString& DisplayName)
{
	// Normalize
	CampaignIndex = FMath::Max(1, CampaignIndex);

	if (RowName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateNewCampaignSave: RowName is None"));
		return nullptr;
	}

	const FString Slot = UCampaignSave::MakeSlotNameFromRowName(RowName);
	constexpr int32 UserIndex = 0;

	UTimerSubsystem* Timer = GetSubsystem<UTimerSubsystem>();

	// Create new save object
	UCampaignSave* NewSave = Cast<UCampaignSave>(
		UGameplayStatics::CreateSaveGameObject(UCampaignSave::StaticClass())
	);

	if (!NewSave)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateNewCampaignSave: Failed to create SaveGame object"));
		return nullptr;
	}

	// Identity
	NewSave->CampaignIndex = CampaignIndex;
	NewSave->CampaignRowName = RowName;
	NewSave->CampaignDisplayName = DisplayName;

	// Reset campaign timeline by anchoring to current universe time
	const uint64 NowUniverse = Timer ? Timer->GetUniverseTimeSeconds() : 0ULL;
	NewSave->InitializeCampaignClock(NowUniverse);

	// Persist (overwrites existing slot for this campaign row)
	const bool bOK = UGameplayStatics::SaveGameToSlot(NewSave, Slot, UserIndex);

	UE_LOG(LogTemp, Warning,
		TEXT("CreateNewCampaignSave: slot=%s ok=%d row=%s index=%d start=%llu"),
		*Slot,
		bOK ? 1 : 0,
		*RowName.ToString(),
		CampaignIndex,
		(unsigned long long)NewSave->CampaignStartUniverseSeconds);

	// Assign + inject into timer subsystem so UI starts at T+ 00:00:00
	CampaignSave = NewSave;

	if (Timer)
	{
		Timer->SetCampaignSave(NewSave);
	}

	return NewSave;
}

UCampaignSave* USSWGameInstance::LoadOrCreateSelectedCampaignSave()
{
	return LoadOrCreateCampaignSave(SelectedCampaignIndex, SelectedCampaignRowName, SelectedCampaignDisplayName);
}

void USSWGameInstance::EnsureCampaignSaveLoaded()
{
	// If already loaded, nothing to do
	if (CampaignSave)
		return;

	int32 UiIndex = PlayerInfo.Campaign;
	if (UiIndex < 0)
	{
		UiIndex = 0;
		PlayerInfo.Campaign = 0;
	}

	SelectedCampaignRowName = NAME_None;
	SelectedCampaignDisplayName = TEXT("");

	if (CampaignDataTable)
	{
		TArray<FName> RowNames = CampaignDataTable->GetRowNames();

		struct FTempCampaignRef
		{
			int32 Index0 = 0;
			FName RowName = NAME_None;
			FString Name;
			bool bAvailable = false;
		};

		TArray<FTempCampaignRef> Sorted;
		Sorted.Reserve(RowNames.Num());

		for (const FName& RN : RowNames)
		{
			const FS_Campaign* Row = CampaignDataTable->FindRow<FS_Campaign>(RN, TEXT("EnsureCampaignSaveLoaded"));
			if (!Row) continue;

			FTempCampaignRef Ref;
			Ref.Index0 = Row->Index;
			Ref.RowName = RN;
			Ref.Name = Row->Name;
			Ref.bAvailable = Row->bAvailable;
			Sorted.Add(Ref);
		}

		Sorted.Sort([](const FTempCampaignRef& A, const FTempCampaignRef& B)
			{
				return A.Index0 < B.Index0;
			});

		UiIndex = FMath::Clamp(UiIndex, 0, Sorted.Num() - 1);

		if (Sorted.IsValidIndex(UiIndex))
		{
			SelectedCampaignRowName = Sorted[UiIndex].RowName;
			SelectedCampaignDisplayName = Sorted[UiIndex].Name;
			SelectedCampaignIndex = Sorted[UiIndex].Index0 + 1;
		}
	}
	else
	{
		if (CampaignData.Num() > 0)
		{
			UiIndex = FMath::Clamp(UiIndex, 0, CampaignData.Num() - 1);
			SelectedCampaignDisplayName = CampaignData[UiIndex].Name;
			SelectedCampaignIndex = UiIndex + 1;
		}
	}

	if (SelectedCampaignIndex < 1)
	{
		//ShowCampaignScreen();
		return;
	}

	// ---- Load/Create the campaign save ----
	LoadOrCreateSelectedCampaignSave();

	// If load failed, do notxproceed to Operations (avoid crash)
	if (!CampaignSave)
	{
		UE_LOG(LogTemp, Error, TEXT("EnsureCampaignSaveLoaded: Failed to load/create CampaignSave"));
		//ShowCampaignScreen();
		return;
	}

	// ---------------------------------------------------------
	// NEW: Inject the loaded campaign save into the TimerSubsystem
	// ---------------------------------------------------------
	if (UTimerSubsystem* Timer = GetSubsystem<UTimerSubsystem>())
	{
		Timer->SetCampaignSave(CampaignSave);
	}
}

AMusicController* USSWGameInstance::GetMusicController()
{
	if (MusicController && IsValid(MusicController))
	{
		return MusicController;
	}

	// Try to find an existing one in the world
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AMusicController> It(World); It; ++It)
		{
			MusicController = *It;
			return MusicController;
		}
	}

	// Optional: spawn if you have a class to spawn
	// If you don’t have one, leave it null and just guard calls.
	return nullptr;
}

FString USSWGameInstance::GetCampaignTPlusString() const
{
	const UTimerSubsystem* Timer = UGameInstance::GetSubsystem<UTimerSubsystem>();
	if (CampaignSave)
	{
		// Uses CampaignSave anchor + UniverseTimeSeconds
		return CampaignSave->GetTPlusDisplay(Timer->UniverseTimeSeconds);
	}

	// notxloaded yet
	return TEXT("T+ --/--:--:--");
}

FString USSWGameInstance::GetCampaignAndUniverseTimeLine() const
{
	const UTimerSubsystem* Timer = GetSubsystem<UTimerSubsystem>();

	const FString UniverseStr = Timer
		? Timer->GetUniverseDateTimeString()
		: TEXT("--");

	const FString TPlusStr = GetCampaignTPlusString(); // your existing method, still fine

	return FString::Printf(
		TEXT("UNIVERSE: %s   |   T+ %s"),
		*UniverseStr,
		*TPlusStr
	);
}

void USSWGameInstance::HandleUniverseMinuteAutosave(uint64 UniverseSecondsNow)
{
	// Universe autosave (your existing method)
	SaveUniverse();

	// Campaign autosave (only if you actually have mutable campaign state)
	SaveCampaign();
}