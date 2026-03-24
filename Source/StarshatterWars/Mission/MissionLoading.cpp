// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "MissionLoading.h"
#include "StarshatterPlayerSubsystem.h"
#include "FormatUtil.h"

void UMissionLoading::GetSelectedMissionData()
{
	USSWGameInstance * SSWInstance = (USSWGameInstance*)GetGameInstance();
	SelectedMission = SSWInstance->PlayerInfo.Mission;
}

void UMissionLoading::NativeConstruct()
{
    Super::NativeConstruct();

    UGameInstance* GIBase = GetGameInstance();
    if (!GIBase)
        return;

    // Player profile (subsystem authoritative)
    UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
        return;

    // Campaign runtime / active campaign (still on GI in current architecture)
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GIBase);
    if (!SSWInstance)
        return;

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
        Timer->OnUniverseSecond.AddUObject(this, &UMissionLoading::HandleUniverseSecondTick);
        Timer->OnUniverseMinute.AddUObject(this, &UMissionLoading::HandleUniverseMinuteTick);

        // Initial push:
        const uint64 Now = Timer->GetUniverseTimeSeconds();
        HandleUniverseSecondTick(Now);

        // If campaign T+ depends on universe seconds, keep this:
        if (SSWInstance->CampaignSave)
        {
            HandleCampaignTPlusChanged(Now, SSWInstance->CampaignSave->GetTPlusSeconds(Now));
        }
    }

    if (TitleText)
        TitleText->SetText(FText::FromString("Mission Briefing").ToUpper());

    if (CancelButton)
    {
        //CancelButton->OnClicked.AddDynamic(this, &UMissionLoading::OnCancelButtonClicked);
        CancelButton->OnHovered.AddDynamic(this, &UMissionLoading::OnCancelButtonHovered);
        CancelButton->OnUnhovered.AddDynamic(this, &UMissionLoading::OnCancelButtonUnHovered);
    }

    if (CancelButtonText)
    {
        CancelButtonText->SetText(FText::FromString("BACK"));
    }

    if (SelectButton)
    {
        SelectButton->OnClicked.AddDynamic(this, &UMissionLoading::OnSelectButtonClicked);
        SelectButton->OnHovered.AddDynamic(this, &UMissionLoading::OnSelectButtonHovered);
        SelectButton->OnUnhovered.AddDynamic(this, &UMissionLoading::OnSelectButtonUnHovered);
    }

    if (SituationButton)
    {
        SituationButton->OnClicked.AddDynamic(this, &UMissionLoading::OnSituationButtonClicked);
        SituationButton->OnHovered.AddDynamic(this, &UMissionLoading::OnSituationButtonHovered);
        SituationButton->OnUnhovered.AddDynamic(this, &UMissionLoading::OnSituationButtonUnHovered);
    }

    if (PackageButton)
    {
        PackageButton->OnClicked.AddDynamic(this, &UMissionLoading::OnPackageButtonClicked);
        PackageButton->OnHovered.AddDynamic(this, &UMissionLoading::OnPackageButtonHovered);
        PackageButton->OnUnhovered.AddDynamic(this, &UMissionLoading::OnPackageButtonUnHovered);
    }

    if (MapButton)
    {
        MapButton->OnClicked.AddDynamic(this, &UMissionLoading::OnMapButtonClicked);
        MapButton->OnHovered.AddDynamic(this, &UMissionLoading::OnMapButtonHovered);
        MapButton->OnUnhovered.AddDynamic(this, &UMissionLoading::OnMapButtonUnHovered);
    }

    if (WeaponsButton)
    {
        WeaponsButton->OnClicked.AddDynamic(this, &UMissionLoading::OnWeaponsButtonClicked);
        WeaponsButton->OnHovered.AddDynamic(this, &UMissionLoading::OnWeaponsButtonHovered);
        WeaponsButton->OnUnhovered.AddDynamic(this, &UMissionLoading::OnWeaponsButtonUnHovered);
    }

    if (PlayButtonText)
    {
        PlayButtonText->SetText(FText::FromString("PLAY"));
    }

    if (MissionScreenSwitcher)
    {
        MissionScreenSwitcher->SetActiveWidgetIndex(0);
    }

    // -------------------------------
    // Campaign + mission lookup
    // -------------------------------
    const auto& ActiveCampaign = SSWInstance->GetActiveCampaign();

    const int32 MissionIndex = PlayerInfo.Mission;

    if (CampaignNameText)
    {
        CampaignNameText->SetText(FText::FromString(ActiveCampaign.Name));
    }

    // Guard MissionIndex bounds; your previous code assumed it was always valid
    if (ActiveCampaign.MissionList.IsValidIndex(MissionIndex))
    {
        const auto& Mission = ActiveCampaign.MissionList[MissionIndex];

        if (MissionNameText)
        {
            MissionNameText->SetText(FText::FromString(Mission.Name));
        }

        if (SystemLocationText)
        {
            SystemLocationText->SetText(FText::FromString(Mission.System));
        }

        if (RegionLocationText)
        {
            RegionLocationText->SetText(FText::FromString(Mission.Region));
        }

        if (MissionStartTimeText)
        {
            MissionStartTimeText->SetText(FText::FromString(FormatDayFromString(Mission.Start)));
        }
    }
    else
    {
        // Defensive UI fallback
        if (MissionNameText)        MissionNameText->SetText(FText::FromString(TEXT("UNKNOWN MISSION")));
        if (SystemLocationText)    SystemLocationText->SetText(FText::FromString(TEXT("UNKNOWN")));
        if (RegionLocationText)    RegionLocationText->SetText(FText::FromString(TEXT("UNKNOWN")));
        if (MissionStartTimeText)  MissionStartTimeText->SetText(FText::FromString(TEXT("")));
    }

    // Player name from subsystem
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));
    }
}


void UMissionLoading::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
}

void UMissionLoading::NativeDestruct()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>())
		{
			Timer->OnUniverseSecond.RemoveAll(this);
			Timer->OnUniverseMinute.RemoveAll(this);
			Timer->OnMissionSecond.RemoveAll(this);
			Timer->OnCampaignTPlusChanged.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void UMissionLoading::OnSelectButtonClicked()
{
}

void UMissionLoading::OnSelectButtonHovered()
{
}

void UMissionLoading::OnSelectButtonUnHovered()
{
}

void UMissionLoading::OnSituationButtonClicked()
{
	if (MissionScreenSwitcher) {
		MissionScreenSwitcher->SetActiveWidgetIndex(0);
	}
}

void UMissionLoading::OnSituationButtonHovered()
{
}

void UMissionLoading::OnSituationButtonUnHovered()
{
}

void UMissionLoading::OnPackageButtonClicked()
{
	if (MissionScreenSwitcher) {
		MissionScreenSwitcher->SetActiveWidgetIndex(1);
	}
}

void UMissionLoading::OnPackageButtonHovered()
{
}

void UMissionLoading::OnPackageButtonUnHovered()
{
}

void UMissionLoading::OnMapButtonClicked()
{
	if (MissionScreenSwitcher) {
		MissionScreenSwitcher->SetActiveWidgetIndex(2);
	}
}

void UMissionLoading::OnMapButtonHovered()
{
}

void UMissionLoading::OnMapButtonUnHovered()
{
}

void UMissionLoading::OnWeaponsButtonClicked()
{
	if (MissionScreenSwitcher) {
		MissionScreenSwitcher->SetActiveWidgetIndex(3);
	}
}

void UMissionLoading::OnWeaponsButtonHovered()
{
}

void UMissionLoading::OnWeaponsButtonUnHovered()
{
}

void UMissionLoading::OnCancelButtonHovered()
{
}

void UMissionLoading::OnCancelButtonUnHovered()
{
}

void UMissionLoading::HandleUniverseSecondTick(uint64 UniverseSecondsNow)
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

void UMissionLoading::HandleUniverseMinuteTick(uint64 UniverseSecondsNow)
{
	USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
	if (!GI) return;

	// Refresh Intel only if Intel page is currently visible
	// In your code Intel index appears to be 3 (OperationalSwitcher->SetActiveWidgetIndex(3) in LoadIntelInfo)

}

void UMissionLoading::HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds)
{
	USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
	if (!GI || !GI->CampaignSave) return;

	if (CampaignTPlusText)
	{
		const FString TPlusString = "T+ " + GI->CampaignSave->GetTPlusDisplay(UniverseSecondsNow);
		CampaignTPlusText->SetText(FText::FromString(TPlusString));
	}
}




