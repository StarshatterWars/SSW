// /*  Project nGenEx  Fractal Dev Games  Copyright (C) 2024. All Rights Reserved.
//     SUBSYSTEM:    SSW
//     FILE:         CampaignScreen.cpp
//     AUTHOR:       Carlos Bott */

#include "CampaignScreen.h"

#include "TimerSubsystem.h"
#include "SSWGameInstance.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "CampaignSave.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"

void UCampaignScreen::NativeConstruct()
{
	Super::NativeConstruct();

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] NativeConstruct: GameInstance is NULL"));
		return;
	}

	UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
	if (!PlayerSS)
	{
		UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] NativeConstruct: PlayerSubsystem is NULL"));
		return;
	}

	// Player save should already be loaded by Boot, but allow a safe fallback:
	if (!PlayerSS->HasLoaded())
	{
		PlayerSS->LoadPlayer();
	}

	const FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetPlayerInfo();

	UStarshatterGameDataSubsystem* DataSubsystem =
		GI->GetSubsystem<UStarshatterGameDataSubsystem>();

	if (!DataSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[CampaignScreen] NativeConstruct: GameDataSubsystem is NULL"));
		return;
	}

	const TArray<FS_Campaign>& Campaigns = DataSubsystem->GetAllCampaigns();
	UE_LOG(LogTemp, Log, TEXT("[CampaignScreen] NativeConstruct: Campaign count = %d"), Campaigns.Num());

	if (TitleText)
	{
		TitleText->SetText(FText::FromString(TEXT("DYNAMIC CAMPAIGNS")));
	}

	// Buttons
	if (CancelButton)
	{
		//CancelButton->OnClicked.RemoveDynamic(this, &UCampaignScreen::OnCancelButtonClicked);
		//CancelButton->OnClicked.AddDynamic(this, &UCampaignScreen::OnCancelButtonClicked);

		CancelButton->OnHovered.RemoveDynamic(this, &UCampaignScreen::OnCancelButtonHovered);
		CancelButton->OnHovered.AddDynamic(this, &UCampaignScreen::OnCancelButtonHovered);

		CancelButton->OnUnhovered.RemoveDynamic(this, &UCampaignScreen::OnCancelButtonUnHovered);
		CancelButton->OnUnhovered.AddDynamic(this, &UCampaignScreen::OnCancelButtonUnHovered);

		if (CancelButtonText)
		{
			CancelButtonText->SetText(FText::FromString(TEXT("CANCEL")));
		}
	}

	if (PlayButton)
	{
		PlayButton->OnClicked.RemoveDynamic(this, &UCampaignScreen::OnPlayButtonClicked);
		PlayButton->OnClicked.AddDynamic(this, &UCampaignScreen::OnPlayButtonClicked);

		PlayButton->OnHovered.RemoveDynamic(this, &UCampaignScreen::OnPlayButtonHovered);
		PlayButton->OnHovered.AddDynamic(this, &UCampaignScreen::OnPlayButtonHovered);

		PlayButton->OnUnhovered.RemoveDynamic(this, &UCampaignScreen::OnPlayButtonUnHovered);
		PlayButton->OnUnhovered.AddDynamic(this, &UCampaignScreen::OnPlayButtonUnHovered);

		if (PlayButtonText)
		{
			// Will be updated by UpdateCampaignButtons()
			PlayButtonText->SetText(FText::FromString(TEXT("START")));
		}
	}

	if (RestartButton)
	{
		RestartButton->OnClicked.RemoveDynamic(this, &UCampaignScreen::OnRestartButtonClicked);
		RestartButton->OnClicked.AddDynamic(this, &UCampaignScreen::OnRestartButtonClicked);

		RestartButton->OnHovered.RemoveDynamic(this, &UCampaignScreen::OnRestartButtonHovered);
		RestartButton->OnHovered.AddDynamic(this, &UCampaignScreen::OnRestartButtonHovered);

		RestartButton->OnUnhovered.RemoveDynamic(this, &UCampaignScreen::OnRestartButtonUnHovered);
		RestartButton->OnUnhovered.AddDynamic(this, &UCampaignScreen::OnRestartButtonUnHovered);

		if (RestartButtonText)
		{
			RestartButtonText->SetText(FText::FromString(TEXT("RESTART")));
		}
	}

	// Dropdown
	if (CampaignSelectDD)
	{
		CampaignSelectDD->OnSelectionChanged.RemoveDynamic(this, &UCampaignScreen::OnSetSelected);
		CampaignSelectDD->OnSelectionChanged.AddDynamic(this, &UCampaignScreen::OnSetSelected);
	}

	// Player name
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));
		UE_LOG(LogTemp, Log, TEXT("[CampaignScreen] Player Name: %s"), *PlayerInfo.Name);
	}

	// Build dropdown options from GameDataSubsystem cache
	SetCampaignDDList();

	// Restore selection: PlayerInfo.Campaign is ALWAYS 1-based campaign index
	int32 SelectedOptionIndex = 0;
	if (PlayerInfo.Campaign > 0)
	{
		const int32 Found = CampaignIndexByOptionIndex.IndexOfByKey(PlayerInfo.Campaign);
		if (Found != INDEX_NONE)
		{
			SelectedOptionIndex = Found;
		}
	}

	Selected = SelectedOptionIndex;

	PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
		? CampaignRowNamesByOptionIndex[Selected]
		: NAME_None;

	if (CampaignSelectDD && CampaignRowNamesByOptionIndex.IsValidIndex(SelectedOptionIndex))
	{
		// Programmatic selection; OnSetSelected should ignore Direct if needed
		CampaignSelectDD->SetSelectedIndex(SelectedOptionIndex);
	}

	// Update right panel and buttons
	SetSelectedData(Selected);
	UpdateCampaignButtons();

	UE_LOG(LogTemp, Log, TEXT("[CampaignScreen] NativeConstruct: Selected=%d Row=%s"),
		Selected,
		*PickedRowName.ToString());
}

UTexture2D* UCampaignScreen::LoadTextureFromFile()
{
	USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
	if (!GI)
	{
		return nullptr;
	}

	return GI->LoadPNGTextureFromFile(ImagePath);
}

FSlateBrush UCampaignScreen::CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = ImageSize;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	return Brush;
}

void UCampaignScreen::OnPlayButtonClicked()
{
	PlayUISound(this, AcceptSound);

	UGameInstance* GIBase = GetGameInstance();
	if (!GIBase)
		return;

	// Player save subsystem (authoritative for PlayerInfo persistence)
	UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
	if (!PlayerSS)
		return;

	// Campaign runtime / legacy routing (still on GI in current architecture)
	USSWGameInstance* GI = Cast<USSWGameInstance>(GIBase);
	if (!GI)
		return;

	if (PickedRowName.IsNone())
		return;

	// Resolve stable campaign index (1-based) from dropdown selection
	const int32 CampaignIndex1Based =
		CampaignIndexByOptionIndex.IsValidIndex(Selected) ? CampaignIndexByOptionIndex[Selected] : (Selected + 1);

	// Build selection metadata FIRST (authoritative)
	GI->SelectedCampaignDisplayName = CampaignSelectDD ? CampaignSelectDD->GetSelectedOption() : TEXT("");
	GI->SelectedCampaignIndex = CampaignIndex1Based; // 1-based
	GI->SelectedCampaignRowName = PickedRowName;

	// Persist selection: PlayerInfo.Campaign is 1-based stable id (SUBSYSTEM ONLY)
	{
		// Boot should have loaded already; safe fallback:
		if (!PlayerSS->HasLoaded())
		{
			PlayerSS->LoadPlayer();
		}

		FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetMutablePlayerInfo();
		PlayerInfo.Campaign = CampaignIndex1Based;
		PlayerInfo.CampaignRowName = PickedRowName; // keep consistent with your struct
		PlayerSS->SavePlayer(true);
	}

	// Check save existence using RowName slot convention
	const bool bHasSave = DoesSelectedCampaignSaveExist();

	// Load/create save
	if (bHasSave)
	{
		GI->LoadOrCreateSelectedCampaignSave();
	}
	else
	{
		GI->CreateNewCampaignSave(
			GI->SelectedCampaignIndex,
			GI->SelectedCampaignRowName,
			GI->SelectedCampaignDisplayName
		);
	}

	// Point timer at active save
	if (UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>())
	{
		Timer->SetCampaignSave(GI->CampaignSave);

		// Only reset time for NEW campaign
		if (!bHasSave)
		{
			Timer->RestartCampaignClock(true);
		}
	}

	GI->ShowCampaignLoading();
}

void UCampaignScreen::OnRestartButtonClicked()
{
	PlayUISound(this, AcceptSound);

	UGameInstance* GIBase = GetGameInstance();
	if (!GIBase)
		return;

	// Player save subsystem (authoritative for PlayerInfo persistence)
	UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
	if (!PlayerSS)
		return;

	// Keep campaign restart flow where it currently lives (GI) for now
	USSWGameInstance* GI = Cast<USSWGameInstance>(GIBase);
	if (!GI)
		return;

	if (PickedRowName.IsNone())
		return;

	// Resolve stable campaign index (1-based)
	const int32 CampaignIndex1Based =
		CampaignIndexByOptionIndex.IsValidIndex(Selected) ? CampaignIndexByOptionIndex[Selected] : (Selected + 1);

	// Authoritative selection metadata (still on GI in your current architecture)
	GI->SelectedCampaignDisplayName = CampaignSelectDD ? CampaignSelectDD->GetSelectedOption() : TEXT("");
	GI->SelectedCampaignIndex = CampaignIndex1Based;
	GI->SelectedCampaignRowName = PickedRowName;

	// Persist selection (1-based) via PlayerSubsystem ONLY
	{
		FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetMutablePlayerInfo();
		PlayerInfo.Campaign = CampaignIndex1Based;
		PlayerInfo.CampaignRowName = PickedRowName; // optional but consistent with your model
		PlayerSS->SavePlayer(true);
	}

	// Overwrite/create campaign save FIRST
	GI->CreateNewCampaignSave(
		GI->SelectedCampaignIndex,
		GI->SelectedCampaignRowName,
		GI->SelectedCampaignDisplayName
	);

	// Restart campaign clock on the new save
	if (UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>())
	{
		Timer->SetCampaignSave(GI->CampaignSave);
		Timer->RestartCampaignClock(true);
	}

	// Restart campaign runtime
	GI->ShowCampaignLoading();
}


void UCampaignScreen::OnPlayButtonHovered()
{
	PlayUISound(this, HoverSound);
}

void UCampaignScreen::OnPlayButtonUnHovered()
{
}

void UCampaignScreen::OnRestartButtonHovered()
{
	PlayUISound(this, HoverSound);
}

void UCampaignScreen::OnRestartButtonUnHovered()
{
}

void UCampaignScreen::OnCancelButtonHovered()
{
	PlayUISound(this, HoverSound);
}

void UCampaignScreen::OnCancelButtonUnHovered()
{
}

void UCampaignScreen::SetCampaignDDList()
{
	USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
	if (!CampaignSelectDD)
		return;

	CampaignSelectDD->ClearOptions();
	CampaignSelectDD->ClearSelection();

	CampaignRowNamesByOptionIndex.Reset();
	CampaignIndexByOptionIndex.Reset();

	const TArray<FName> RowNames = GI->CampaignDataTable->GetRowNames();

	for (const FName& RowName : RowNames)
	{
		const FS_Campaign* Row = GI->CampaignDataTable->FindRow<FS_Campaign>(RowName, TEXT("CampaignScreen"));
		if (!Row || !Row->bAvailable)
			continue;

		CampaignSelectDD->AddOption(Row->Name);

		// Parallel arrays keyed by dropdown option index
		CampaignRowNamesByOptionIndex.Add(RowName);
		CampaignIndexByOptionIndex.Add(Row->Index + 1); // store 1-based stable campaign index
	}

	// NO selection here. NativeConstruct restores selection.
}

void UCampaignScreen::SetSelectedData(int32 OptionIndex)
{
	UGameInstance* GIBase = GetGameInstance();
	if (!GIBase)
		return;

	// Campaign data currently lives on GI in your architecture
	USSWGameInstance* GI = Cast<USSWGameInstance>(GIBase);
	if (!GI)
		return;

	// Player persistence lives in PlayerSubsystem
	UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
	if (!PlayerSS)
		return;

	// OptionIndex is dropdown option index (0-based)
	Selected = OptionIndex;

	if (!GI->CampaignData.IsValidIndex(Selected))
	{
		UE_LOG(LogTemp, Warning, TEXT("SetSelectedData: CampaignData invalid index %d (num=%d)"),
			Selected, GI->CampaignData.Num());
		return;
	}

	// Ensure Orders has 4 entries for UI
	GI->CampaignData[Selected].Orders.SetNum(4);

	GetCampaignImageFile(Selected);

	if (UTexture2D* LoadedTexture = LoadTextureFromFile())
	{
		if (CampaignImage)
		{
			const FSlateBrush Brush = CreateBrushFromTexture(
				LoadedTexture,
				FVector2D(LoadedTexture->GetSizeX(), LoadedTexture->GetSizeY())
			);
			CampaignImage->SetBrush(Brush);
		}
	}

	if (CampaignNameText)
		CampaignNameText->SetText(FText::FromString(GI->CampaignData[Selected].Name));

	if (CampaignStartTimeText)
		CampaignStartTimeText->SetText(FText::FromString(GI->CampaignData[Selected].Start));

	if (DescriptionText)
		DescriptionText->SetText(FText::FromString(GI->CampaignData[Selected].Description));

	if (SituationText)
		SituationText->SetText(FText::FromString(GI->CampaignData[Selected].Situation));

	if (Orders1Text)
		Orders1Text->SetText(FText::FromString(GI->CampaignData[Selected].Orders[0]));

	if (Orders2Text)
		Orders2Text->SetText(FText::FromString(GI->CampaignData[Selected].Orders[1]));

	if (Orders3Text)
		Orders3Text->SetText(FText::FromString(GI->CampaignData[Selected].Orders[2]));

	if (Orders4Text)
		Orders4Text->SetText(FText::FromString(GI->CampaignData[Selected].Orders[3]));

	if (LocationSystemText)
	{
		const FString LocationText = GI->CampaignData[Selected].System + TEXT("/") + GI->CampaignData[Selected].Region;
		LocationSystemText->SetText(FText::FromString(LocationText));
	}

	// Update RowName for save-slot lookups
	PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
		? CampaignRowNamesByOptionIndex[Selected]
		: NAME_None;

	// Persist stable campaign selection (1-based) so opening the screen restores correctly
	const int32 CampaignIndex1Based =
		CampaignIndexByOptionIndex.IsValidIndex(Selected) ? CampaignIndexByOptionIndex[Selected] : (Selected + 1);

	// Boot should have loaded already; safe fallback:
	if (!PlayerSS->HasLoaded())
	{
		PlayerSS->LoadPlayer();
	}

	// SUBSYSTEM ONLY persistence
	{
		FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetMutablePlayerInfo();
		PlayerInfo.Campaign = CampaignIndex1Based;
		PlayerInfo.CampaignRowName = PickedRowName; // keep consistent with your struct intent
		PlayerSS->SavePlayer(true);
	}
}

void UCampaignScreen::OnSetSelected(FString SelectedItem, ESelectInfo::Type Type)
{
	// Ignore programmatic SetSelectedIndex calls
	if (Type == ESelectInfo::Direct)
		return;

	if (!CampaignSelectDD)
		return;

	const int32 NewIndex = CampaignSelectDD->FindOptionIndex(SelectedItem);
	if (NewIndex == INDEX_NONE)
		return;

	Selected = NewIndex;

	PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(NewIndex)
		? CampaignRowNamesByOptionIndex[NewIndex]
		: NAME_None;

	SetSelectedData(NewIndex);
	UpdateCampaignButtons();
}

void UCampaignScreen::GetCampaignImageFile(int32 OptionIndex)
{
	USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
	if (!GI || !GI->CampaignData.IsValidIndex(OptionIndex))
	{
		ImagePath.Empty();
		return;
	}

	// This is UI folder convention based on dropdown ordering (legacy)
	// If you prefer folder to follow campaign index (1-based), switch to CampaignIndexByOptionIndex[OptionIndex].
	ImagePath = FPaths::ProjectContentDir() + TEXT("UI/Campaigns/0");
	ImagePath.Append(FString::FromInt(OptionIndex + 1));
	ImagePath.Append(TEXT("/"));
	ImagePath.Append(GI->CampaignData[OptionIndex].MainImage);
	ImagePath.Append(TEXT(".png"));

	UE_LOG(LogTemp, Log, TEXT("Campaign Image: %s"), *ImagePath);
}

void UCampaignScreen::PlayUISound(UObject* WorldContext, USoundBase* UISound)
{
	if (UISound)
	{
		UGameplayStatics::PlaySound2D(WorldContext, UISound);
	}
}

bool UCampaignScreen::DoesSelectedCampaignSaveExist() const
{
	const USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
	if (!GI)
		return false;

	if (PickedRowName.IsNone())
		return false;

	const FString GameSlot = UCampaignSave::MakeSlotNameFromRowName(PickedRowName);
	constexpr int32 UserIndex = 0;
	return UGameplayStatics::DoesSaveGameExist(GameSlot, UserIndex);
}

void UCampaignScreen::UpdateCampaignButtons()
{
	const bool bHasSave = DoesSelectedCampaignSaveExist();

	if (PlayButton)
	{
		PlayButton->SetIsEnabled(true);
	}

	if (PlayButtonText)
	{
		PlayButtonText->SetText(FText::FromString(bHasSave ? TEXT("CONTINUE") : TEXT("START")));
	}

	if (RestartButton)
	{
		RestartButton->SetIsEnabled(bHasSave);
	}
}
