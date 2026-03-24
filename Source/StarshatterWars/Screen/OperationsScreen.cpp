// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OperationsScreen.h"
#include "MissionListObject.h"
#include "IntelListObject.h"
#include "RosterTVElement.h"
#include "Components/ListView.h"
#include "MenuButton.h"
#include "FormattingUtils.h"

#include "OOBForceItem.h"
#include "OOBFleetItem.h"
#include "OOBCarrierGroupItem.h" 
#include "OOBBattleItem.h"
#include "OOBDestroyerItem.h"
#include "OOBWingItem.h"
#include "OOBUnitItem.h"
#include "OOBSquadronItem.h"
#include "OOBFighterUnit.h"
#include "OOBFighterSquadronItem.h"
#include "OOBBattalion.h"
#include "OOBCivilianItem.h"
#include "OOBBatteryItem.h"
#include "OOBForceWidget.h"
#include "SystemMarker.h"
#include "GalaxyLink.h"
#include "GalaxyMap.h"
#include "SystemMap.h"
#include "SectorMap.h"
#include "GalaxyManager.h"

#include "SelectableButtonGroup.h"
#include "MenuButton.h"
#include "StarshatterPlayerSubsystem.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

template<typename TEnum>
bool FStringToEnum(const FString& InString, TEnum& OutEnum, bool bCaseSensitive = true)
{
	UEnum* Enum = StaticEnum<TEnum>();
	if (!Enum) return false;

	for (int32 i = 0; i < Enum->NumEnums(); ++i)
	{
		FString Name = Enum->GetNameStringByIndex(i);
		if ((bCaseSensitive && Name == InString) ||
			(!bCaseSensitive && Name.Equals(InString, ESearchCase::IgnoreCase)))
		{
			OutEnum = static_cast<TEnum>(Enum->GetValueByIndex(i));
			return true;
		}
	}
	return true;
}

static FString FormatTPlus(uint64 TPlusSeconds)
{
	const uint64 Days = TPlusSeconds / 86400ULL;
	const uint64 Rem = TPlusSeconds % 86400ULL;
	const uint64 Hours = Rem / 3600ULL;
	const uint64 Minutes = (Rem % 3600ULL) / 60ULL;
	const uint64 Seconds = Rem % 60ULL;

	return FString::Printf(TEXT("T+ %02llu/%02llu:%02llu:%02llu"),
		(unsigned long long)Days,
		(unsigned long long)Hours,
		(unsigned long long)Minutes,
		(unsigned long long)Seconds);
}

void UOperationsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	UGameInstance* GIBase = GetGameInstance();
	if (!GIBase) return;

	// Player profile (subsystem authoritative)
	UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
	if (!PlayerSS) return;

	// Campaign runtime / legacy routing (still on GI in current architecture)
	USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GIBase);
	if (!SSWInstance) return;

	// Boot should have loaded already; safe fallback:
	if (!PlayerSS->HasLoaded())
	{
		PlayerSS->LoadPlayer();
	}

	const FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetPlayerInfo();

	// -------------------------------
	// PUB/SUB SUBSCRIBE
	// -------------------------------
	UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>();

	if (Timer)
	{
		Timer->OnUniverseSecond.AddUObject(this, &UOperationsScreen::HandleUniverseSecondTick);
		Timer->OnUniverseMinute.AddUObject(this, &UOperationsScreen::HandleUniverseMinuteTick);
		Timer->OnCampaignTPlusChanged.AddUObject(this, &UOperationsScreen::HandleCampaignTPlusChanged);

		// Initial push:
		const uint64 Now = Timer->GetUniverseTimeSeconds();
		HandleUniverseSecondTick(Now);

		// If campaign T+ depends on universe seconds, keep this:
		if (SSWInstance->CampaignSave)
		{
			HandleCampaignTPlusChanged(Now, SSWInstance->CampaignSave->GetTPlusSeconds(Now));
		}
	}

	CombatantList = SSWInstance->GetCombatantList();

	GetCurrentCarrierGroup();

	if (TitleText)
		TitleText->SetText(FText::FromString("Operational Command").ToUpper());

	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &UOperationsScreen::OnCancelButtonClicked);
		CancelButton->OnHovered.AddDynamic(this, &UOperationsScreen::OnCancelButtonHovered);
	}

	if (CancelButtonText)
	{
		CancelButtonText->SetText(FText::FromString("BACK"));
	}

	// Active campaign theater defaults (still stored on GI in your current flow)
	SSWInstance->SelectedSystem = SSWInstance->GetActiveCampaign().System;
	SSWInstance->SelectedSector.Name = SSWInstance->GetActiveCampaign().Region;

	if (TheaterGalaxyButton)
	{
		TheaterGalaxyButton->OnSelected.AddDynamic(this, &UOperationsScreen::OnTheaterGalaxyButtonSelected);
		TheaterGalaxyButton->OnHovered.AddDynamic(this, &UOperationsScreen::OnTheaterGalaxyButtonHovered);

		if (UTextBlock* Label = Cast<UTextBlock>(TheaterGalaxyButton->GetWidgetFromName("Label")))
		{
			Label->SetText(FText::FromString("GALAXY"));
		}
	}

	if (TheaterSystemButton)
	{
		TheaterSystemButton->OnSelected.AddDynamic(this, &UOperationsScreen::OnTheaterSystemButtonSelected);
		TheaterSystemButton->OnHovered.AddDynamic(this, &UOperationsScreen::OnTheaterSystemButtonHovered);

		if (UTextBlock* Label = Cast<UTextBlock>(TheaterSystemButton->GetWidgetFromName("Label")))
		{
			Label->SetText(FText::FromString("SYSTEM"));
		}
	}

	if (TheaterSectorButton)
	{
		TheaterSectorButton->OnSelected.AddDynamic(this, &UOperationsScreen::OnTheaterSectorButtonSelected);
		TheaterSectorButton->OnHovered.AddDynamic(this, &UOperationsScreen::OnTheaterSectorButtonHovered);

		if (UTextBlock* Label = Cast<UTextBlock>(TheaterSectorButton->GetWidgetFromName("Label")))
		{
			Label->SetText(FText::FromString("SECTOR"));
		}
	}

	if (CurrentLocationText)
	{
		CurrentLocationText->SetText(
			FText::FromString(SSWInstance->GetActiveCampaign().System + " System").ToUpper());
	}

	if (EmpireSelectionDD)
	{
		EmpireSelectionDD->ClearOptions();
		EmpireSelectionDD->OnSelectionChanged.AddDynamic(this, &UOperationsScreen::OnSetEmpireSelected);
	}

	if (AudioButton)
	{
		AudioButton->OnClicked.AddDynamic(this, &UOperationsScreen::OnAudioButtonClicked);
	}

	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &UOperationsScreen::OnSelectButtonClicked);
		SelectButton->OnHovered.AddDynamic(this, &UOperationsScreen::OnSelectButtonHovered);
	}

	if (PlayButtonText)
	{
		PlayButtonText->SetText(FText::FromString("SELECT"));
	}

	if (!MenuButtonClass || !MenuToggleGroup || !MenuButtonContainer) return;

	for (int32 i = 0; i < 5; ++i)
	{
		UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
		if (!NewButton) continue;

		if (UTextBlock* Label = Cast<UTextBlock>(NewButton->GetWidgetFromName("Label")))
		{
			Label->SetText(FText::FromString(MenuItems[i]).ToUpper());
		}
		NewButton->MenuOption = MenuItems[i];

		MenuButtonContainer->AddChild(NewButton);
		MenuToggleGroup->RegisterButton(NewButton);

		NewButton->OnSelected.AddDynamic(this, &UOperationsScreen::OnMenuToggleSelected);
		NewButton->OnHovered.AddDynamic(this, &UOperationsScreen::OnMenuToggleHovered);
		AllMenuButtons.Add(NewButton);
	}

	// Player name from subsystem
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));
	}

	if (OperationalSwitcher)
	{
		OperationalSwitcher->SetActiveWidgetIndex(0);
	}

	if (MapSwitcher)
	{
		MapSwitcher->SetActiveWidgetIndex(0);
	}

	if (OperationsModeText)
	{
		OperationsModeText->SetText(FText::FromString("ORDERS"));
	}

	ActiveCampaign = SSWInstance->GetActiveCampaign();

	if (SSWInstance->GetActiveCampaign().Index == 0)
	{
		if (TheaterButton)  TheaterButton->SetIsEnabled(false);
		if (ForcesButton)   ForcesButton->SetIsEnabled(false);
		if (IntelButton)    IntelButton->SetIsEnabled(false);
	}
	else
	{
		if (TheaterButton)  TheaterButton->SetIsEnabled(true);
		if (ForcesButton)   ForcesButton->SetIsEnabled(true);
		if (IntelButton)    IntelButton->SetIsEnabled(true);
	}

	if (ForceListView)
	{
		ForceListView->ClearListItems();

		for (const FS_OOBForce& Force : LoadedForces)
		{
			UOOBForceItem* ForceItem = NewObject<UOOBForceItem>(this);
			ForceItem->Data = Force;
			ForceListView->AddItem(ForceItem);
		}
	}

	SelectedMission = 0;

	if (AllMenuButtons.Num() > 0)
	{
		AllMenuButtons[0]->SetSelected(true);
	}

	ScreenOffset.X = 600;
	ScreenOffset.Y = 300;

	CreateGalaxyMap();
	SetCampaignOrders();
	PopulateMissionList();
	PopulateEmpireDDList();
	PopulateIntelList();
	SetCampaignMissions();
	LoadForces(UFormattingUtils::GetEmpireTypeFromIndex(0));
	CreateSystemMap(SSWInstance->SelectedSystem.ToUpper());

	const FS_OOBWing* Wing = FindWingForCarrierGroup(CurrentCarrierGroup, LoadedForces);

	if (CurrentUnitText && Wing != nullptr)
	{
		CurrentUnitText->SetText(FText::FromString(Wing->Name).ToUpper());
	}
}


void UOperationsScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();

	if (SSWInstance->MissionSelectionChanged) {
		SetSelectedMissionData(SSWInstance->GetSelectedMissionNr());
	}

	if (SSWInstance->ActionSelectionChanged) {
		SetSelectedIntelData(SSWInstance->GetSelectedActionNr());
	}

	if (SSWInstance->bIsDisplayUnitChanged)  {
		HandleUnitClicked();
	}
	if (SSWInstance->bIsDisplayElementChanged) {
		HandleElementClicked();
	}

	if (AudioButton)
	{
		AudioButton->SetIsEnabled(!SSWInstance->IsSoundPlaying());
	}
}

void UOperationsScreen::NativeDestruct()
{
	Super::NativeDestruct();
}

void UOperationsScreen::OnSelectButtonClicked()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);
	SSWInstance->LoadMissionBriefingScreen();
}

void UOperationsScreen::OnSelectButtonHovered()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayHoverSound(this);
}

void UOperationsScreen::LoadForcesInfo()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	if (OperationalSwitcher) {
		OperationalSwitcher->SetActiveWidgetIndex(2);
	}
	if (OperationsModeText) {
		OperationsModeText->SetText(FText::FromString("FORCES"));
	}

	if (InformationBorder) InformationBorder->SetVisibility(ESlateVisibility::Collapsed);

	if (InformationLabel)
	{
		InformationLabel->SetText(FText::FromString(""));
	}
}

void UOperationsScreen::LoadOrdersInfo()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	if (OperationalSwitcher) {
		OperationalSwitcher->SetActiveWidgetIndex(0);
	}
	if (OperationsModeText) {
		OperationsModeText->SetText(FText::FromString("ORDERS"));
	}
	SetCampaignOrders();
}

void UOperationsScreen::LoadMissionsInfo()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	if (OperationalSwitcher) {
		OperationalSwitcher->SetActiveWidgetIndex(4);
	}

	if (OperationsModeText) {
		OperationsModeText->SetText(FText::FromString("MISSIONS"));
	}
	SetCampaignMissions();
	PopulateMissionList();
	SetSelectedMissionData(SSWInstance->GetSelectedMissionNr());
}

void UOperationsScreen::LoadIntelInfo()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	if (OperationalSwitcher) {
		OperationalSwitcher->SetActiveWidgetIndex(3);
	}

	if (OperationsModeText) {
		OperationsModeText->SetText(FText::FromString("INTEL"));
	}

	PopulateIntelList();
	SetSelectedIntelData(SSWInstance->GetSelectedActionNr());
}

void UOperationsScreen::LoadTheaterInfo()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	if (OperationalSwitcher) {
		OperationalSwitcher->SetActiveWidgetIndex(1);
	}

	if (OperationsModeText) {
		OperationsModeText->SetText(FText::FromString("THEATER"));
	}
	
	
}

void UOperationsScreen::OnAudioButtonClicked()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	SSWInstance->PlaySoundFromFile(AudioPath);
	//if(AudioAsset)
	//{
	//	UGameplayStatics::PlaySound2D(GetWorld(), AudioAsset);
	//}
}

void UOperationsScreen::OnCancelButtonClicked()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	SSWInstance->ShowMainMenuScreen();
}

void UOperationsScreen::OnCancelButtonHovered()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayHoverSound(this);
}

void UOperationsScreen::SetCampaignOrders()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();

	if (CampaignNameText) {
		CampaignNameText->SetText(FText::FromString(SSWInstance->GetActiveCampaign().Name));
	}

	if (DescriptionText) {
		DescriptionText->SetText(FText::FromString(SSWInstance->GetActiveCampaign().Description));
	}

	if (SituationText) {
		SituationText->SetText(FText::FromString(SSWInstance->GetActiveCampaign().Situation));
	}

	if (Orders1Text) {

		Orders1Text->SetText(FText::FromString(SSWInstance->GetActiveCampaign().Orders[0]));
	}
	if (Orders2Text) {

		Orders2Text->SetText(FText::FromString(SSWInstance->GetActiveCampaign().Orders[1]));
	}
	if (Orders3Text) {

		Orders3Text->SetText(FText::FromString(SSWInstance->GetActiveCampaign().Orders[2]));
	}
	if (Orders4Text) {

		Orders4Text->SetText(FText::FromString(SSWInstance->GetActiveCampaign().Orders[3]));
	}
	if (LocationSystemText) {
		FString LocationSystem = SSWInstance->GetActiveCampaign().System + "/" + SSWInstance->GetActiveCampaign().Region;
		LocationSystemText->SetText(FText::FromString(LocationSystem));
	}
}

void UOperationsScreen::SetCampaignMissions()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();

	TArray<FS_CampaignMissionList> Missions = SSWInstance->GetActiveCampaign().MissionList;

	for (FS_CampaignMissionList info : Missions)
	{
		FString NewMission = info.Name;
		//UE_LOG(LogTemp, Log, TEXT("Mission Name Selected: %s"), *NewMission);
	}
}

void UOperationsScreen::PopulateMissionList()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();

	if (!MissionList) return;

	MissionList->ClearListItems();

	for (int32 i = 0; i < SSWInstance->GetActiveCampaign().MissionList.Num(); ++i)
	{
		UMissionListObject* ListItem = NewObject<UMissionListObject>();
		ListItem->MissionName = SSWInstance->GetActiveCampaign().MissionList[i].Name;
		ListItem->MissionType = SSWInstance->GetActiveCampaign().MissionList[i].TypeName;
		ListItem->MissionTime = SSWInstance->GetActiveCampaign().MissionList[i].Start;
		ListItem->MissionRegion = SSWInstance->GetActiveCampaign().MissionList[i].Region;
		ListItem->MissionSystem = SSWInstance->GetActiveCampaign().MissionList[i].System;
		ListItem->MissionSitrep = SSWInstance->GetActiveCampaign().MissionList[i].System;

		if (SSWInstance->GetActiveCampaign().MissionList[i].Status == EMISSIONSTATUS::Active) {
			ListItem->MissionStatus = "Active";
		}
		else if (SSWInstance->GetActiveCampaign().MissionList[i].Status == EMISSIONSTATUS::Complete) {
			ListItem->MissionStatus = "Complete";
		}
		else if (SSWInstance->GetActiveCampaign().MissionList[i].Status == EMISSIONSTATUS::Ready) {
			ListItem->MissionStatus = "Ready";
		}
		else if (SSWInstance->GetActiveCampaign().MissionList[i].Status == EMISSIONSTATUS::Available) {
			ListItem->MissionStatus = "Available";
		}
		else if (SSWInstance->GetActiveCampaign().MissionList[i].Status == EMISSIONSTATUS::Pending) {
			ListItem->MissionStatus = "Pending";
		}

		MissionList->GetIndexForItem(ListItem);
		MissionList->AddItem(ListItem);

		//UE_LOG(LogTemp, Log, TEXT("Ops Mission Name: %s"), *ListItem->MissionName);
	}
}

void UOperationsScreen::PopulateIntelList()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();

	if (!IntelList) return;

	IntelList->ClearListItems();
	ActionList.Empty();

	for (int32 i = 0; i < SSWInstance->GetActiveCampaign().Action.Num(); ++i)
	{
		if (SSWInstance->GetActiveCampaign().Action[i].Type == "event") {
			if (SSWInstance->GetActiveCampaign().Action[i].Date <= SSWInstance->GetCampaignTPlusString()) { // Show only Active Actions
			UIntelListObject* ListItem = NewObject<UIntelListObject>();
			FS_CampaignAction ActiveAction;

			ListItem->NewsTitle = SSWInstance->GetActiveCampaign().Action[i].Title;
			ActiveAction.Title = SSWInstance->GetActiveCampaign().Action[i].Title;

			ListItem->NewsLocation = SSWInstance->GetActiveCampaign().Action[i].Region;
			ActiveAction.Region = SSWInstance->GetActiveCampaign().Action[i].Region;

			ListItem->NewsSource = SSWInstance->GetActiveCampaign().Action[i].Source;
			ActiveAction.Source = SSWInstance->GetActiveCampaign().Action[i].Source;

			ListItem->NewsDate = SSWInstance->GetActiveCampaign().Action[i].Date;
			ActiveAction.Date = SSWInstance->GetActiveCampaign().Action[i].Date;

			ListItem->NewsInfoText = SSWInstance->GetActiveCampaign().Action[i].Message;
			ActiveAction.Message = SSWInstance->GetActiveCampaign().Action[i].Message;

			ListItem->NewsImage = SSWInstance->GetActiveCampaign().Action[i].Image;
			ActiveAction.Image = SSWInstance->GetActiveCampaign().Action[i].Image;

			ListItem->NewsAudio = SSWInstance->GetActiveCampaign().Action[i].Audio;
			ActiveAction.Audio = SSWInstance->GetActiveCampaign().Action[i].Audio;

			ListItem->NewsVisited = false;

			IntelList->GetIndexForItem(ListItem);
			ActionList.Add(ActiveAction);
			IntelList->AddItem(ListItem);
			}
		}
	}
}

FDateTime UOperationsScreen::GetCampaignTime()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	return FDateTime::FromUnixTimestamp(SSWInstance->GetGameTime());
}

void UOperationsScreen::SetSelectedMissionData(int32 Selected)
{
	UGameInstance* GIBase = GetGameInstance();
	if (!GIBase)
		return;

	// Player persistence lives in PlayerSubsystem
	UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
	if (!PlayerSS)
		return;

	// Campaign runtime currently lives on GI
	USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GIBase);
	if (!SSWInstance)
		return;

	// Rename local to avoid hiding class member ActiveCampaign
	const auto& Campaign = SSWInstance->GetActiveCampaign();

	if (!Campaign.MissionList.IsValidIndex(Selected))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SetSelectedMissionData: Invalid mission index %d (num=%d)"),
			Selected, Campaign.MissionList.Num());
		return;
	}

	const FString SelectedMissionName = Campaign.MissionList[Selected].Name;

	if (MissionNameText)
		MissionNameText->SetText(FText::FromString(Campaign.MissionList[Selected].Name));

	if (MissionDescriptionText)
		MissionDescriptionText->SetText(FText::FromString(Campaign.MissionList[Selected].Description));

	if (MissionSitrepText)
	{
		FString SitrepText = Campaign.MissionList[Selected].Sitrep;
		SitrepText = SitrepText.Replace(TEXT("\\n"), TEXT("\n"));
		MissionSitrepText->SetText(FText::FromString(SitrepText));
	}

	if (MissionStartText)
		MissionStartText->SetText(FText::FromString(Campaign.MissionList[Selected].Start));

	if (MissionStatusText)
	{
		const EMISSIONSTATUS Status = Campaign.MissionList[Selected].Status;

		if (Status == EMISSIONSTATUS::Active)     MissionStatusText->SetText(FText::FromString(TEXT("Active")));
		else if (Status == EMISSIONSTATUS::Complete)   MissionStatusText->SetText(FText::FromString(TEXT("Complete")));
		else if (Status == EMISSIONSTATUS::Ready)      MissionStatusText->SetText(FText::FromString(TEXT("Ready")));
		else if (Status == EMISSIONSTATUS::Available)  MissionStatusText->SetText(FText::FromString(TEXT("Available")));
		else if (Status == EMISSIONSTATUS::Pending)    MissionStatusText->SetText(FText::FromString(TEXT("Pending")));
		else                                           MissionStatusText->SetText(FText::FromString(TEXT("Unknown")));
	}

	if (MissionTypeText)
		MissionTypeText->SetText(FText::FromString(Campaign.MissionList[Selected].TypeName));

	if (MissionSystemText)
		MissionSystemText->SetText(FText::FromString(Campaign.MissionList[Selected].System));

	if (MissionRegionText)
		MissionRegionText->SetText(FText::FromString(Campaign.MissionList[Selected].Region));

	if (MissionObjectiveText)
		MissionObjectiveText->SetText(FText::FromString(Campaign.MissionList[Selected].Objective));

	GetMissionImageFile(Selected);

	if (UTexture2D* LoadedTexture = LoadTextureFromFile())
	{
		if (MissionImage)
		{
			const FSlateBrush Brush =
				CreateBrushFromTexture(
					LoadedTexture,
					FVector2D(LoadedTexture->GetSizeX(), LoadedTexture->GetSizeY()));
			MissionImage->SetBrush(Brush);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Ops Mission Selected: %s"), *SelectedMissionName);

	// Legacy flag still lives on GI
	SSWInstance->MissionSelectionChanged = false;

	// Persist selected mission via PlayerSubsystem ONLY
	if (!PlayerSS->HasLoaded())
		PlayerSS->LoadPlayer();

	{
		FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetMutablePlayerInfo();
		PlayerInfo.Mission = Selected;
		PlayerSS->SavePlayer(true);
	}
}


void UOperationsScreen::SetSelectedIntelData(int Selected)
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();

	if (IntelNameText) {
		IntelNameText->SetText(FText::FromString(ActionList[Selected].Title));
	}

	if (IntelLocationText) {
		IntelLocationText->SetText(FText::FromString(ActionList[Selected].Region));
	}

	if (IntelSourceText) {
		IntelSourceText->SetText(FText::FromString(ActionList[Selected].Source));
	}
	if (IntelDateText) {
		IntelDateText->SetText(FText::FromString(ActionList[Selected].Date));
	}

	if (IntelMessageText) {
		FString MessageText = ActionList[Selected].Message;
		MessageText = MessageText.Replace(TEXT("\\n"), TEXT("\n"));

		IntelMessageText->SetText(FText::FromString(MessageText));
	}

	GetIntelImageFile(ActionList[Selected].Image);
	GetIntelAudioFile(ActionList[Selected].Audio);

	UTexture2D* LoadedTexture = LoadTextureFromFile();
	if (LoadedTexture && IntelImage)
	{
		FSlateBrush Brush = CreateBrushFromTexture(LoadedTexture, FVector2D(LoadedTexture->GetSizeX(), LoadedTexture->GetSizeY()));
		IntelImage->SetBrush(Brush);
	}
	SSWInstance->ActionSelectionChanged = false;
}

void UOperationsScreen::GetIntelImageFile(FString IntelImageName)
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	ImagePath = FPaths::ProjectContentDir() + TEXT("UI/Campaigns/0");
	ImagePath.Append(FString::FromInt(SSWInstance->GetActiveCampaign().Index + 1));
	ImagePath.Append("/");
	ImagePath.Append(IntelImageName);
	ImagePath.Append(".png");
	UE_LOG(LogTemp, Log, TEXT("Action Image: %s"), *ImagePath);
}

void UOperationsScreen::GetIntelAudioFile(FString IntelAudioName)
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	AudioPath = "/Game/Audio/Vox/Scenes/0";
	AudioPath.Append(FString::FromInt(SSWInstance->GetActiveCampaign().Index + 1));
	AudioPath.Append("/");
	AudioPath.Append(IntelAudioName);
	AudioPath.Append(".");
	AudioPath.Append(IntelAudioName);

	//SSWInstance->PlaySoundFromFile(AudioPath);
	//AudioAsset = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *AudioPath));

	UE_LOG(LogTemp, Log, TEXT("Action Audio: %s"), *AudioPath);
}

void UOperationsScreen::GetMissionImageFile(int selected)
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	ImagePath = FPaths::ProjectContentDir() + TEXT("UI/Campaigns/0");
	ImagePath.Append(FString::FromInt(SSWInstance->GetActiveCampaign().Index + 1));
	ImagePath.Append("/");
	ImagePath.Append(SSWInstance->GetActiveCampaign().MissionList[selected].MissionImage);
	ImagePath.Append(".png");
	UE_LOG(LogTemp, Log, TEXT("Mission Image: %s"), *ImagePath);
}


UTexture2D* UOperationsScreen::LoadTextureFromFile()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	UTexture2D* LoadedTexture = SSWInstance->LoadPNGTextureFromFile(ImagePath);
	return LoadedTexture;
}

FSlateBrush UOperationsScreen::CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = ImageSize;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	return Brush;
}

FString
UOperationsScreen::GetOrdinal(int id)
{
	FString ordinal;

	int last_two_digits = id % 100;

	if (last_two_digits > 10 && last_two_digits < 20) {
		ordinal = FString::FormatAsNumber(id) + "th";
	}
	else {
		int last_digit = last_two_digits % 10;

		if (last_digit == 1)
			ordinal = FString::FormatAsNumber(id) + "st";
		else if (last_digit == 2)
			ordinal = FString::FormatAsNumber(id) + "nd";
		else if (last_digit == 3)
			ordinal = FString::FormatAsNumber(id) + "rd";
		else
			ordinal = FString::FormatAsNumber(id) + "th";
	}

	return ordinal;
}

void UOperationsScreen::ClearForces()
{
	if (ForceListView) {
		ForceListView->ClearListItems();
	}

	if (InfoBoxPanel) InfoBoxPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (InformationBorder) InformationBorder->SetVisibility(ESlateVisibility::Collapsed);

	if (InfoPanel) InfoPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UOperationsScreen::LoadForces(EEMPIRE_NAME Empire)
{
	USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
	if (!SSWInstance || !SSWInstance->OrderOfBattleDataTable || !ForceListView) return;

	ClearForces();

	LoadedForces.Empty();

	// Step 1: Load matching forces
	TArray<FName> RowNames = SSWInstance->OrderOfBattleDataTable->GetRowNames();

	for (const FName& RowName : RowNames)
	{
		if (FS_OOBForce* Force = SSWInstance->OrderOfBattleDataTable->FindRow<FS_OOBForce>(RowName, TEXT("LoadForces")))
		{
			if ((Force->Intel == EINTEL_TYPE::KNOWN || Force->Intel == EINTEL_TYPE::TRACKED) && Empire == Force->Empire)
			{
				LoadedForces.Add(*Force);
			}
		}
	}

	// Step 2: Filter loaded forces
	FilterOutput(LoadedForces, Empire);

	// Step 3: Add to ListView
	for (const FS_OOBForce& Force : LoadedForces)
	{
		UOOBForceItem* ForceItem = NewObject<UOOBForceItem>(this);
		ForceItem->Data = Force;
		ForceListView->AddItem(ForceItem);
	}
}

void UOperationsScreen::HandleUnitClicked()
{
	USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
	
	FS_DisplayUnit Display = SSWInstance->GetActiveUnit();

	UE_LOG(LogTemp, Log, TEXT("Unit clicked: %s"), *Display.Name);

	if (InfoPanel) InfoPanel->SetVisibility(ESlateVisibility::Visible);

	if (InfoBoxPanel) {
		InfoBoxPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Update UI fields
	if (GroupInfoText)
	{
		GroupInfoText->SetText(FText::FromString(Display.Name));
	}

	if (GroupTypeText)
	{
		GroupTypeText->SetText(FText::FromString(UFormattingUtils::GetGroupTypeDisplayName(Display.Type)));
	}

	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::FromString(Display.Location));
	}

	if (GroupEmpireText)
	{
		GroupEmpireText->SetText(FText::FromString(UFormattingUtils::GetEmpireDisplayName(Display.Empire)));
	}
	SSWInstance->bIsDisplayUnitChanged = false;
	SSWInstance->bIsDisplayElementChanged = false;
}

void UOperationsScreen::HandleElementClicked()
{
	USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());

	FS_DisplayElement Display = SSWInstance->GetActiveElement();

	UE_LOG(LogTemp, Log, TEXT("Element clicked: %s"), *Display.Name);

	if (InfoPanel) InfoPanel->SetVisibility(ESlateVisibility::Visible);

	if (InfoBoxPanel) {
		InfoBoxPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Update UI fields
	if (GroupInfoText)
	{
		GroupInfoText->SetText(FText::FromString(Display.Name));
	}

	if (GroupTypeText)
	{
		GroupTypeText->SetText(FText::FromString(UFormattingUtils::GetUnitTypeDisplayName(Display.Type)));
	}

	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::FromString(Display.Location));
	}

	if (GroupEmpireText)
	{
		GroupEmpireText->SetText(FText::FromString(UFormattingUtils::GetEmpireDisplayName(Display.Empire)));
	}
	SSWInstance->bIsDisplayUnitChanged = false;
	SSWInstance->bIsDisplayElementChanged = false;
}

TArray<FSubGroupArray> UOperationsScreen::GetSubGroupArrays(const FS_OOBFleet& Fleet)
{
	TArray<FSubGroupArray> SubGroups;

	// Battles
	FSubGroupArray BattleArray(ECOMBATGROUP_TYPE::BATTLE_GROUP);
	for (const FS_OOBBattle& Battle : Fleet.Battle)
	{
		BattleArray.Ids.Add(Battle.Id);
	}
	if (BattleArray.Ids.Num() > 0)
	{
		SubGroups.Add(BattleArray);
	}

	// Destroyers
	FSubGroupArray DesronArray(ECOMBATGROUP_TYPE::DESTROYER_SQUADRON);
	for (const FS_OOBDestroyer& Desron : Fleet.Destroyer)
	{
		DesronArray.Ids.Add(Desron.Id);
	}
	if (DesronArray.Ids.Num() > 0)
	{
		SubGroups.Add(DesronArray);
	}

	// Carriers
	FSubGroupArray CarrierArray(ECOMBATGROUP_TYPE::CARRIER_GROUP);
	for (const FS_OOBCarrier& Carrier : Fleet.Carrier)
	{
		CarrierArray.Ids.Add(Carrier.Id);
	}
	if (CarrierArray.Ids.Num() > 0)
	{
		SubGroups.Add(CarrierArray);
	}

	// Carriers
	FSubGroupArray MinefieldlArray(ECOMBATGROUP_TYPE::MINEFIELD);
	for (const FS_OOBMinefield& Minefield : Fleet.Minefield)
	{
		MinefieldlArray.Ids.Add(Minefield.Id);
	}
	if (MinefieldlArray.Ids.Num() > 0)
	{
		SubGroups.Add(MinefieldlArray);
	}

	// Future extensions:
	// Wings, Strike Groups, Task Forces: just add here once

	return SubGroups;
}

TArray<FSubGroupArray> UOperationsScreen::GetBattalionSubGroups(const FS_OOBBattalion& Battalion)
{
	TArray<FSubGroupArray> SubGroups;

	// Battery
	FSubGroupArray BatteryArray(ECOMBATGROUP_TYPE::BATTERY);
	for (const FS_OOBBattery& Battery : Battalion.Battery)
	{
		BatteryArray.Ids.Add(Battery.Id);
	}
	if (BatteryArray.Ids.Num() > 0)
	{
		SubGroups.Add(BatteryArray);
	}

	// Destroyers
	/*FSubGroupArray DesronArray(ECOMBATGROUP_TYPE::DESTROYER_SQUADRON);
	for (const FS_OOBDestroyer& Desron : Fleet.Destroyer)
	{
		DesronArray.Ids.Add(Desron.Id);
	}
	if (DesronArray.Ids.Num() > 0)
	{
		SubGroups.Add(DesronArray);
	}

	// Carriers
	FSubGroupArray CarrierArray(ECOMBATGROUP_TYPE::CARRIER_GROUP);
	for (const FS_OOBCarrier& Carrier : Fleet.Carrier)
	{
		CarrierArray.Ids.Add(Carrier.Id);
	}
	if (CarrierArray.Ids.Num() > 0)
	{
		SubGroups.Add(CarrierArray);
	}
	*/
	// Future extensions:
	// Wings, Strike Groups, Task Forces: just add here once

	return SubGroups;
}

void UOperationsScreen::FilterOutput(TArray<FS_OOBForce>& ForcesTable, EEMPIRE_NAME EmpireFilter)
{
	USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
	if (!SSWInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid GameInstance!"));
		return;
	}

	const TArray<FS_Combatant>& Combatants = SSWInstance->GetCombatantList();
	TSet<FMatchedGroupKey> MatchedIds;

	// --- Matching ---
	for (const FS_OOBForce& Force : ForcesTable)
	{
		if (EmpireFilter != EEMPIRE_NAME::NONE && Force.Empire != EmpireFilter)
		{
			continue;
		}

		// Match Fleets
		for (const FS_OOBFleet& Fleet : Force.Fleet)
		{
			MatchCombatantGroups(Force.Empire, Fleet.Id, Fleet.Type, Combatants, MatchedIds);

			TArray<FSubGroupArray> SubGroups = GetSubGroupArrays(Fleet);
			for (const FSubGroupArray& GroupArray : SubGroups)
			{
				for (int32 SubId : GroupArray.Ids)
				{
					MatchCombatantGroups(Force.Empire, SubId, GroupArray.Type, Combatants, MatchedIds);
				}
			}
		}

		// Match Battalions
		for (const FS_OOBBattalion& Battalion : Force.Battalion)
		{
			MatchCombatantGroups(Force.Empire, Battalion.Id, ECOMBATGROUP_TYPE::BATTALION, Combatants, MatchedIds);

			TArray<FSubGroupArray> SubGroups = GetBattalionSubGroups(Battalion);
			for (const FSubGroupArray& GroupArray : SubGroups)
			{
				for (int32 SubId : GroupArray.Ids)
				{
					MatchCombatantGroups(Force.Empire, SubId, GroupArray.Type, Combatants, MatchedIds);
				}
			}
		}

		// Match Civilians
		for (const FS_OOBCivilian& Civilian : Force.Civilian)
		{
			MatchCombatantGroups(Force.Empire, Civilian.Id, ECOMBATGROUP_TYPE::CIVILIAN, Combatants, MatchedIds);
		}
	}

	// --- Deletion ---
	for (FS_OOBForce& Force : ForcesTable)
	{
		// Clean up Fleets
		for (FS_OOBFleet& Fleet : Force.Fleet)
		{
			bool bFleetMatched = MatchedIds.Contains(FMatchedGroupKey(Force.Empire, Fleet.Type, Fleet.Id));

			if (!bFleetMatched)
			{
				Fleet.Battle.RemoveAll([&](const FS_OOBBattle& Battle)
					{
						return !MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::BATTLE_GROUP, Battle.Id));
					});

				Fleet.Destroyer.RemoveAll([&](const FS_OOBDestroyer& Desron)
					{
						return !MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::DESTROYER_SQUADRON, Desron.Id));
					});

				Fleet.Carrier.RemoveAll([&](const FS_OOBCarrier& Carrier)
					{
						return !MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::CARRIER_GROUP, Carrier.Id));
					});
			}
		}

		// Remove empty Fleets
		Force.Fleet.RemoveAll([&](const FS_OOBFleet& Fleet)
			{
				bool bFleetMatched = MatchedIds.Contains(FMatchedGroupKey(Force.Empire, Fleet.Type, Fleet.Id));
				bool bFleetHasSubordinates = (Fleet.Battle.Num() > 0 || Fleet.Destroyer.Num() > 0 || Fleet.Carrier.Num() > 0);

				return !bFleetMatched && !bFleetHasSubordinates;
			});

		// Clean up Battalions
		for (FS_OOBBattalion& Battalion : Force.Battalion)
		{
			bool bBattalionMatched = MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::BATTALION, Battalion.Id));

			if (!bBattalionMatched)
			{
				Battalion.Battery.RemoveAll([&](const FS_OOBBattery& Battery)
					{
						return !MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::BATTERY, Battery.Id));
					});

				Battalion.Starbase.RemoveAll([&](const FS_OOBStarbase& Starbase)
					{
						return !MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::STARBASE, Starbase.Id));
					});

				Battalion.Station.RemoveAll([&](const FS_OOBStation& Station)
					{
						return !MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::STATION, Station.Id));
					});
			}
		}

		// Remove empty Battalions
		Force.Battalion.RemoveAll([&](const FS_OOBBattalion& Battalion)
			{
				bool bBattalionMatched = MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::BATTALION, Battalion.Id));
				bool bBattalionHasSubordinates = (Battalion.Battery.Num() > 0 || Battalion.Starbase.Num() > 0 || Battalion.Station.Num() > 0);

				return !bBattalionMatched && !bBattalionHasSubordinates;
			});

		// Remove unmatched Civilians
		Force.Civilian.RemoveAll([&](const FS_OOBCivilian& Civilian)
			{
				return !MatchedIds.Contains(FMatchedGroupKey(Force.Empire, ECOMBATGROUP_TYPE::CIVILIAN, Civilian.Id));
			});
	}
}

void UOperationsScreen::MatchCombatantGroups(
	EEMPIRE_NAME Empire,
	int32 SubId,
	ECOMBATGROUP_TYPE SubType,
	const TArray<FS_Combatant>& Combatants,
	TSet<FMatchedGroupKey>& MatchedIds)
{
	for (const FS_Combatant& Combatant : Combatants)
	{
		if (Empire == Combatant.Name)
		{
			for (const FS_CombatantGroup& Group : Combatant.Group)
			{
				if (Group.Type == SubType && Group.Id == SubId)
				{
					MatchedIds.Add(FMatchedGroupKey(Empire, SubType, SubId));
				}
			}
		}
	}
}

void UOperationsScreen::OnSetEmpireSelected(FString SelectedItem, ESelectInfo::Type type) {
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SelectedEmpire = EEMPIRE_NAME::NONE;

	if (type == ESelectInfo::OnMouseClick) {

		if (UEnum* EmpireEnum = StaticEnum<EEMPIRE_NAME>())
		{
			int32 NumEnums = EmpireEnum->NumEnums();
			for (int32 i = 0; i < NumEnums - 1; ++i) // Skip _MAX usually
			{
				int64 EnumValue = EmpireEnum->GetValueByIndex(i);
				EEMPIRE_NAME Empire = static_cast<EEMPIRE_NAME>(EnumValue);

				FText DisplayName = EmpireEnum->GetDisplayNameTextByIndex(i);
				if (DisplayName.ToString() == SelectedItem)
				{
					SelectedEmpire = Empire;

					UE_LOG(LogTemp, Log, TEXT("Selected Empire: %s (%d)"), *DisplayName.ToString(), (int32)SelectedEmpire);
					break;
				}
			}
		}
	}
	LoadForces(SelectedEmpire);
}

void UOperationsScreen::OnMenuToggleHovered(UMenuButton* HoveredButton)
{
	if (!HoveredButton) return;

	UE_LOG(LogTemp, Log, TEXT("Hovered over: %s"), *HoveredButton->MenuOption);
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayHoverSound(this);
}

void UOperationsScreen::OnTheaterGalaxyButtonSelected(UMenuButton* SelectedButton)
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	ShowGalaxyMap();
}

void UOperationsScreen::OnTheaterGalaxyButtonHovered(UMenuButton* HoveredButton)
{
	if (!HoveredButton) return;

	UE_LOG(LogTemp, Log, TEXT("Hovered over: %s"), *HoveredButton->MenuOption);
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayHoverSound(this);
}
void UOperationsScreen::OnTheaterSystemButtonSelected(UMenuButton* SelectedButton)
{
	ShowSystemMap();
}

void UOperationsScreen::ShowGalaxyMap() {
	
	if (MapSwitcher) {
		MapSwitcher->SetActiveWidgetIndex(0);
	}
}

void UOperationsScreen::ShowSystemMap() {

	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	if (MapSwitcher) {
		MapSwitcher->SetActiveWidgetIndex(1);
	}

	if (SystemNameText) {
		SystemNameText->SetText(FText::FromString(SSWInstance->SelectedSystem.ToUpper() + " SYSTEM"));
	}
	CreateSystemMap(SSWInstance->SelectedSystem.ToUpper());
}


void UOperationsScreen::ShowSectorMap(FS_PlanetMap Planet) {

	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	if (MapSwitcher) {
		MapSwitcher->SetActiveWidgetIndex(2);
	}

	if (SectorNameText) {
		SectorNameText->SetText(FText::FromString(Planet.Name.ToUpper() + " SECTOR"));
	}
	CreateSectorMap(Planet.Name.ToUpper());
}


void UOperationsScreen::OnTheaterSystemButtonHovered(UMenuButton* HoveredButton)
{
	if (!HoveredButton) return;

	UE_LOG(LogTemp, Log, TEXT("Hovered over: %s"), *HoveredButton->MenuOption);
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayHoverSound(this);
}

void UOperationsScreen::OnTheaterSectorButtonSelected(UMenuButton* SelectedButton)
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	if (MapSwitcher) {
		MapSwitcher->SetActiveWidgetIndex(2);
	}

	if (SectorNameText) {
		SectorNameText->SetText(FText::FromString(SSWInstance->SelectedSector.Name.ToUpper() + " SECTOR"));
	}
}

void UOperationsScreen::OnTheaterSectorButtonHovered(UMenuButton* HoveredButton)
{
	if (!HoveredButton) return;

	UE_LOG(LogTemp, Log, TEXT("Hovered over: %s"), *HoveredButton->MenuOption);
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayHoverSound(this);
}

void UOperationsScreen::OnMenuToggleSelected(UMenuButton* SelectedButton)
{
	if (!SelectedButton) return;

	// Example action when one of the buttons is selected
	UE_LOG(LogTemp, Log, TEXT("Selected button: %s"), *GetNameSafe(SelectedButton));

	// Example: compare button text, tags, or use a mapping
	// Then trigger OOB filtering or changes in UI accordingly
	const FString& MenuOption = SelectedButton->MenuOption;

	if (MenuOption == MenuItems[0])
	{
		LoadOrdersInfo();
	}
	else if (MenuOption == MenuItems[1])
	{
		LoadTheaterInfo();
	}
	else if (MenuOption == MenuItems[2])
	{
		LoadForcesInfo();
	}
	else if (MenuOption == MenuItems[3])
	{
		LoadIntelInfo();
	}
	else if (MenuOption == MenuItems[4])
	{
		LoadMissionsInfo();
	}
}

void UOperationsScreen::OnMenuButtonSelected(UMenuButton* SelectedButton)
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->PlayAcceptSound(this);

	for (UMenuButton* Button : AllMenuButtons)
	{
		if (Button)
		{
			Button->SetSelected(Button == SelectedButton);
		}
	}
}

void UOperationsScreen::PopulateEmpireDDList()
{
	if (!EmpireSelectionDD) return;
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	TArray<FS_Combatant> Combatant = SSWInstance->GetActiveCampaign().Combatant;

	EmpireSelectionDD->ClearOptions();

	TSet<EEMPIRE_NAME> AvailableEmpires;
	for (FS_Combatant Item : Combatant) {
		if (Item.Name != EEMPIRE_NAME::NONE) {
			AvailableEmpires.Add(Item.Name);
		}
	}

	if (UEnum* EmpireEnum = StaticEnum<EEMPIRE_NAME>())
	{
		for (int32 i = 0; i < EmpireEnum->NumEnums() - 1; ++i)
		{
			int64 EnumValue = EmpireEnum->GetValueByIndex(i);
			EEMPIRE_NAME Empire = static_cast<EEMPIRE_NAME>(EnumValue);

			if (AvailableEmpires.Contains(Empire))
			{
				FText DisplayName = EmpireEnum->GetDisplayNameTextByIndex(i);
				EmpireSelectionDD->AddOption(DisplayName.ToString());
			}
		}
	}

	if (EmpireSelectionDD->GetOptionCount() > 0)
	{
		EmpireSelectionDD->SetSelectedOption(EmpireSelectionDD->GetOptionAtIndex(0));
	}
}

void UOperationsScreen::CreateGalaxyMap() {
	UGalaxyMap* GalaxyMap = CreateWidget<UGalaxyMap>(this, MapClass);
	GalaxyMap->SetOwner(this); // Assign the owner screen
	if (!MapClass) return;

	GalaxyMapCanvas->AddChildToCanvas(GalaxyMap);
	
	if (UCanvasPanelSlot* MapSlot = GalaxyMapCanvas->AddChildToCanvas(GalaxyMap))
	{
		MapSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f)); // Stretch to all edges
		MapSlot->SetOffsets(FMargin(0.f));                 // No padding
		MapSlot->SetAlignment(FVector2D(0.f, 0.f));        // Top-left corner alignment
		MapSlot->SetZOrder(2);
	}
	//GalaxyMap->SetVisibility(ESlateVisibility::Collapsed);
}

void UOperationsScreen::CreateSystemMap(FString Name) {
	
	UE_LOG(LogTemp, Log, TEXT("UOperationsScreen::CreateSystemMap() Called %s"), *Name);
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	
	if (SystemMap) {
		SystemMap->ClearSystemView();
		SystemMap->InitMapCanvas();
	}
	if (!SystemMap)
	{
		SystemMap = CreateWidget<USystemMap>(this, SystemMapClass);
		SystemMap->SetOwner(this); // Assign owner
		SystemMap->SetFocus(); // keyboard
		SystemMap->SetUserFocus(GetOwningPlayer()); // controller
		SystemMap->SetKeyboardFocus(); // optional redundancy
		SystemMap->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		SystemMap->InitMapCanvas();
	}
	
	if (!SystemMapClass) {
		UE_LOG(LogTemp, Log, TEXT("UOperationsScreen::CreateSystemMap() Widget notxCreaed"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("UOperationsScreen::CreateSystemMap() Widget Created"));
	
	SystemMapCanvas->AddChildToCanvas(SystemMap);

	if (UCanvasPanelSlot* MapSlot = SystemMapCanvas->AddChildToCanvas(SystemMap))
	{
		MapSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f)); // Stretch to all edges
		MapSlot->SetOffsets(FMargin(0.f));                 // No padding
		MapSlot->SetAlignment(FVector2D(0.f, 0.f));        // Top-left corner alignment
		MapSlot->SetZOrder(2);
	}
}

void UOperationsScreen::CreateSectorMap(FString Name) {
	
	UE_LOG(LogTemp, Log, TEXT("UOperationsScreen::CreateSectorMap() Called %s"), *Name);
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	if (!SectorMap)
	{
		SectorMap = CreateWidget<USectorMap>(this, SectorMapClass);
		SectorMap->SetOwner(this); // Assign owner
		SectorMap->SetFocus(); // keyboard
		SectorMap->SetUserFocus(GetOwningPlayer()); // controller
		SectorMap->SetKeyboardFocus(); // optional redundancy
		SectorMap->SetClipping(EWidgetClipping::ClipToBoundsAlways);  // strict
		SectorMap->InitSectorCanvas();
	}

	if (!SectorMapClass) {
		UE_LOG(LogTemp, Log, TEXT("UOperationsScreen::CreateSectorMap() Widget notxCreaed"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UOperationsScreen::CreateSectorMap() Widget Created"));

	SectorMapCanvas->AddChildToCanvas(SectorMap);

	if (UCanvasPanelSlot* MapSlot = SectorMapCanvas->AddChildToCanvas(SectorMap))
	{
		MapSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f)); // Stretch to all edges
		MapSlot->SetOffsets(FMargin(0.f));                 // No padding
		MapSlot->SetAlignment(FVector2D(0.f, 0.f));        // Top-left corner alignment
		MapSlot->SetZOrder(2);
	}
}


const FS_OOBWing* UOperationsScreen::FindWingForCarrierGroup(int CarrierGroupId, const TArray<FS_OOBForce>& AllForces)
{
	for (const FS_OOBForce& Force : AllForces)
	{
		for (const FS_OOBFleet& Fleet : Force.Fleet)
		{
			for (const FS_OOBCarrier& Carrier : Fleet.Carrier)
			{
				if (Carrier.Id == CarrierGroupId)
				{
					// Return the first wing attached to this carrier
					if (Carrier.Wing.Num() > 0)
					{
						UE_LOG(LogTemp, Log, TEXT("Selected carrier wing: %d"), Carrier.Wing[0].Id);
						return &Carrier.Wing[0]; // or loop through all if needed
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("Selected carrier wing notxfound"));
						return nullptr; // no wings attached
					}
				}
			}
		}
	}
	return nullptr; // carrier group notxfound
}
void UOperationsScreen::GetCurrentCarrierGroup() 
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	TArray<FS_Combatant> Combatants = SSWInstance->GetCombatantList();
	for (const FS_Combatant& Combatant : Combatants)
	{
		if (Combatant.Name == EEMPIRE_NAME::Terellian)
		{
			TArray<FS_CombatantGroup> CurrentGroups = Combatant.Group;
			for (const FS_CombatantGroup& CombatantGroup : CurrentGroups)
			{
				if (CombatantGroup.Type == ECOMBATGROUP_TYPE::CARRIER_GROUP) {
					CurrentCarrierGroup = CombatantGroup.Id;
					UE_LOG(LogTemp, Log, TEXT("Selected carrier group: %d"), CurrentCarrierGroup);
					break;
				}
			}
		}
	}
}

void UOperationsScreen::HandleUniverseSecondTick(uint64 UniverseSecondsNow)
{
	USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
	if (!GI) return;

	UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
	if (!Timer) return;

	if (GameTimeText)
	{
		GameTimeText->SetText(
			FText::FromString(Timer->GetUniverseDateTimeString())
		);
	}
}

void UOperationsScreen::HandleUniverseMinuteTick(uint64 UniverseSecondsNow)
{
	USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
	if (!GI) return;

	// Refresh Intel only if Intel page is currently visible
	// In your code Intel index appears to be 3 (OperationalSwitcher->SetActiveWidgetIndex(3) in LoadIntelInfo)
	if (OperationalSwitcher && OperationalSwitcher->GetActiveWidgetIndex() == 3)
	{
		PopulateIntelList();

		// If you want to keep selection stable:
		const int32 Sel = GI->GetSelectedActionNr();
		if (Sel >= 0 && Sel < ActionList.Num())
		{
			SetSelectedIntelData(Sel);
		}
	}
}

void UOperationsScreen::HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds)
{
	if (!CampaignTPlusText) return;

	CampaignTPlusText->SetText(FText::FromString(FormatTPlus(TPlusSeconds)));
}

