// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h"

#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/WidgetSwitcher.h"
  
#include "Components/ListView.h"

#include "Kismet/GameplayStatics.h"
#include "SSWGameInstance.h"
#include "TimerSubsystem.h"
#include "OperationsScreen.generated.h"

// Forward declarations
class UListView;

class UOOBForceItem;
class UOOBFleetItem;
class UOOBCarrierGroupItem;
class UOOBBattleItem;
class UOOBDestroyerItem;
class UOOBWingItem;
class UOOBUnitItem;
class UOOBSquadronItem;
class UOOBFighterSquadronItem;
class UOOBFighterUnit;
class UOOBBattalion;
class UOOBBatteryItem;
class UOOBCivilianItem;
class UOOBForceWidget;

class UPanelWidget;
class USelectableButtonGroup;
class UMenuButton;
class UVerticalBox;
class UScrollBox;

class USystemMarker;
class UGalaxyLink;
class UGalaxyMap;
class USystemMap;
class USectorMap;
class UTexture2D;
class ACentralSunActor;

struct FSubGroupArray
{
	ECOMBATGROUP_TYPE Type;
	TArray<int32> Ids;

	FSubGroupArray(ECOMBATGROUP_TYPE InType)
		: Type(InType)
	{
	}
};

USTRUCT()
struct FMatchedGroupKey
{
    GENERATED_BODY()

    UPROPERTY()
    EEMPIRE_NAME Empire;

    UPROPERTY()
    ECOMBATGROUP_TYPE Type;

    UPROPERTY()
    int32 Id;

    FMatchedGroupKey()
        : Empire(EEMPIRE_NAME::NONE)
        , Type(ECOMBATGROUP_TYPE::NONE)
        , Id(INDEX_NONE)
    {}

    FMatchedGroupKey(EEMPIRE_NAME InEmpire, ECOMBATGROUP_TYPE InType, int32 InId)
        : Empire(InEmpire)
        , Type(InType)
        , Id(InId)
    {}

    bool operator==(const FMatchedGroupKey& Other) const
    {
        return Empire == Other.Empire && Type == Other.Type && Id == Other.Id;
    }
};

// Required to use in TSet/TMap
FORCEINLINE uint32 GetTypeHash(const FMatchedGroupKey& Key)
{
    uint32 Hash = HashCombine(::GetTypeHash((uint8)Key.Empire), ::GetTypeHash((uint8)Key.Type));
    return HashCombine(Hash, ::GetTypeHash(Key.Id));
}

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOperationsScreen : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PlayerNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GameTimeText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CampaignTPlusText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GalaxyNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SystemNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SectorNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CampaignNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DescriptionText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SituationText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Orders1Text;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Orders2Text;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Orders3Text;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Orders4Text;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* LocationSystemText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionDescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionSitrepText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionStartText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionStatusText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionTypeText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionSystemText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionRegionText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionObjectiveText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* IntelNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* IntelSourceText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* IntelLocationText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* IntelMessageText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* IntelDateText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GroupInfoText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GroupLocationText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GroupTypeText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GroupEmpireText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GroupInformationText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* OperationsModeText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PlayButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CancelButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GroupInfomationText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* InformationLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CurrentUnitText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CurrentLocationText;

	
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SelectButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* CancelButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* MissionImage;
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* IntelImage;
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* GroupImage;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* OrdersButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* TheaterButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* ForcesButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* IntelButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* MissionsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UMenuButton* TheaterGalaxyButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UMenuButton* TheaterSystemButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UMenuButton* TheaterSectorButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* AudioButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UComboBoxString* EmpireSelectionDD;

	UPROPERTY(meta = (BindWidgetOptional))
	class UBorder* InformationBorder;
	UPROPERTY(meta = (BindWidgetOptional))
	class UCanvasPanel* InfoPanel;
	UPROPERTY(meta = (BindWidgetOptional))
	class UCanvasPanel* InfoBoxPanel;

	 // Canvas panel for placing icons
    UPROPERTY(meta = (BindWidgetOptional))
    class UCanvasPanel* GalaxyMapCanvas;
	 // Canvas panel for placing icons
    UPROPERTY(meta = (BindWidgetOptional))
    class UCanvasPanel* SystemMapCanvas;
	 // Canvas panel for placing icons
    UPROPERTY(meta = (BindWidgetOptional))
    class UCanvasPanel* SectorMapCanvas;

	// Parent container to hold Forces
    UPROPERTY(meta = (BindWidgetOptional))
    UScrollBox* ForcesScrollBox;
		
	UPROPERTY(meta = (BindWidgetOptional))
	class UWidgetSwitcher* OperationalSwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	class UWidgetSwitcher* MapSwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	UListView* MissionList;

	UPROPERTY(meta = (BindWidgetOptional))
	UListView* IntelList;
	
	UPROPERTY(meta = (BindWidgetOptional))
	int SelectedMission;

	UPROPERTY()
	USoundBase* AudioAsset;

	UPROPERTY(meta = (BindWidgetOptional))
	USelectableButtonGroup* MenuToggleGroup;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* MenuButtonContainer;

public:
	UFUNCTION()
	void SetSelectedMissionData(int Selected);
	UFUNCTION()
	void SetSelectedIntelData(int Selected);
	UFUNCTION()
	void LoadForces(EEMPIRE_NAME Empire);
	UFUNCTION()
	void HandleUnitClicked();
	UFUNCTION()
	void HandleElementClicked();
	TArray<FSubGroupArray> GetSubGroupArrays(const FS_OOBFleet& Fleet);
	TArray<FSubGroupArray> GetBattalionSubGroups(const FS_OOBBattalion& Battalion);
	void FilterOutput(TArray<FS_OOBForce>& Forces, EEMPIRE_NAME Empire);

	void MatchCombatantGroups(EEMPIRE_NAME Empire, int32 SubId, ECOMBATGROUP_TYPE SubType, const TArray<FS_Combatant>& Combatants, TSet<FMatchedGroupKey>& MatchedIds); // now using FMatchedGroupKey
	UFUNCTION()
	void OnSetEmpireSelected(FString dropDownInt, ESelectInfo::Type type);
	UFUNCTION()
	void OnMenuToggleHovered(UMenuButton* SelectedButton);
	UFUNCTION()
	void OnTheaterGalaxyButtonSelected(UMenuButton* SelectedButton);
	UFUNCTION()
	void OnTheaterGalaxyButtonHovered(UMenuButton* HoveredButton);
	UFUNCTION()
	void OnTheaterSystemButtonSelected(UMenuButton* SelectedButton);
	UFUNCTION()
	void ShowGalaxyMap();
	UFUNCTION()
	void ShowSystemMap();
	UFUNCTION()
	void ShowSectorMap(FS_PlanetMap Planet);
	UFUNCTION()
	void OnTheaterSystemButtonHovered(UMenuButton* HoveredButton);
	UFUNCTION()
	void OnTheaterSectorButtonSelected(UMenuButton* SelectedButton);
	UFUNCTION()
	void OnTheaterSectorButtonHovered(UMenuButton* HoveredButton);
	UFUNCTION()
	void OnMenuToggleSelected(UMenuButton* SelectedButton);
	UFUNCTION()
	void OnMenuButtonSelected(UMenuButton* SelectedButton);
	void PopulateEmpireDDList();;
	UFUNCTION()
	void CreateGalaxyMap();
	UFUNCTION()
	void CreateSystemMap(FString Name);
	UFUNCTION()
	void CreateSectorMap(FString Name);
	UFUNCTION()
	void GetCurrentCarrierGroup();

	const FS_OOBWing* FindWingForCarrierGroup(int CarrierGroupId, const TArray<FS_OOBForce>& AllForces);

	UPROPERTY(meta = (BindWidgetOptional))
	UListView* ForceListView;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UMenuButton> MenuButtonClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FS_OOBForce> LoadedForces;
	
	// Scaling factor (optional)
    UPROPERTY()
    float MapScale = 70.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D ScreenOffset;
	
	USystemMarker* SelectedMarker = nullptr;

protected:
	//UOperationsScreen(const FObjectInitializer& ObjectInitializer);

	void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UTexture2D* LoadTextureFromFile();
	FSlateBrush CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize);
	UFUNCTION()
	FString GetOrdinal(int id);
	UFUNCTION()
	void ClearForces();
	UFUNCTION()
	void GetIntelImageFile(FString IntelImageName);
	UFUNCTION()
	void GetIntelAudioFile(FString IntelAudioName);
	UFUNCTION()
	void GetMissionImageFile(int selected);
	UFUNCTION()
	void OnSelectButtonClicked();
	UFUNCTION()
	void OnSelectButtonHovered();

	UFUNCTION()
	void OnCancelButtonClicked();
	UFUNCTION()
	void OnCancelButtonHovered();
	
	UFUNCTION()
	void LoadForcesInfo();
	UFUNCTION()
	void LoadOrdersInfo();
	UFUNCTION()
	void LoadMissionsInfo();
	UFUNCTION()
	void LoadIntelInfo();
	UFUNCTION()
	void LoadTheaterInfo();
	
	// Store all buttons created
	UPROPERTY()
	TArray<UMenuButton*> AllMenuButtons;

	UFUNCTION()
	void OnAudioButtonClicked();
	UFUNCTION()
	void SetCampaignOrders();
	UFUNCTION()
	void SetCampaignMissions();
	UFUNCTION()
	void PopulateMissionList();
	UFUNCTION()
	void PopulateIntelList();

	UFUNCTION()
	FDateTime GetCampaignTime();

	 // TSubclassOf must be set in UMG (or via C++)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TSubclassOf<UGalaxyMap> MapClass;

	 // TSubclassOf must be set in UMG (or via C++)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TSubclassOf<USystemMap> SystemMapClass;

	 // TSubclassOf must be set in UMG (or via C++)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TSubclassOf<USectorMap> SectorMapClass;

private:
	FS_Campaign ActiveCampaign;

	UPROPERTY()
	FString ImagePath;

	UPROPERTY()
	FString AudioPath;

	UPROPERTY()
	TArray<FS_CampaignAction> ActionList;

	UPROPERTY()
	TArray<FS_Combatant> CombatantList;
	
	UPROPERTY()
	TArray<FString> MenuItems = {
		TEXT("ORDERS"),
		TEXT("THEATER"),
		TEXT("FORCES"),
		TEXT("INTEL"),
		TEXT("MISSIONS")
	};

	TArray<FString> EmpireDDItems;
	EEMPIRE_NAME SelectedEmpire; 
	int IndentLevel = 0;
	int CurrentCarrierGroup; 
	TSubclassOf<UGalaxyMap> GalaxyMapClass;
	USystemMap* SystemMap =	nullptr;
	USectorMap* SectorMap = nullptr;

	void HandleUniverseSecondTick(uint64 UniverseSecondsNow);
	void HandleUniverseMinuteTick(uint64 UniverseSecondsNow);
	void HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds);
};

static FString FormatTPlus(uint64 TPlusSeconds);
