#pragma once
#include "Widgets/SCompoundWidget.h"
#include "NativeHUDSymbology.h"
class SScrollBox;
class SVerticalBox;
#include "DamagePanelData.h"
#include "ShipMFDPreview.h"
#include "CockpitDisplays.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"

class AActor;
class FNativeHUDSymbology;
class WeaponGroup;
class ASSWSessionLobbyController;
struct FMissionCameraRig;
class FShipMFDPreview;
class FCockpitDisplays;
class SCockpitInstrument;

// Live fighter readouts. Simulation objects are sampled, never retained.
class SFighterHUDDetails : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFighterHUDDetails) : _ThirdPerson(false) {}
        SLATE_ATTRIBUTE(bool, ShowHUD)
        SLATE_ATTRIBUTE(bool, ThirdPerson)
        SLATE_ATTRIBUTE(FString, CameraMode)
        SLATE_ATTRIBUTE(TWeakObjectPtr<AActor>, SelectedTarget)
        SLATE_ATTRIBUTE(FText, SelectedName)
        SLATE_ARGUMENT(TWeakObjectPtr<ASSWSessionLobbyController>, NetworkController)
        SLATE_EVENT(TDelegate<bool(float)>, OnOverviewWheel)
    SLATE_END_ARGS()
    virtual ~SFighterHUDDetails();
    void Construct(const FArguments& Args);
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual void Tick(const FGeometry&, double Now, float Delta) override;
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
        FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    static FLinearColor IdentificationColor(int32 ContactIFF,int32 ObserverIFF)
    {
        return ContactIFF<0 ? FLinearColor(0.45f,0.45f,0.45f) :
            ContactIFF==0 ? FLinearColor(1.f,0.85f,0.f) :
            ContactIFF==ObserverIFF ? FLinearColor(0.25f,0.65f,1.f) : FLinearColor::Red;
    }
    virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
    void BuildRadioRows(TArray<FString>& Labels, TArray<int32>& Commands, TArray<bool>& Enabled, FString& Title) const;
    void ToggleRadio();
    void RadioSelect(int32 Number);
    bool IsRadioOpen() const { return RadioPage >= 0 || IsCameraPanelOpen(); }
    bool IsTravelSpeedOpen() const { return RadioPage == 8; }
    bool IsCameraPanelOpen() const { return CameraPage != ECameraPage::Closed; }
    void ToggleCameraPanel(const TSharedPtr<FMissionCameraRig>& Camera);
    TOptional<int32> SelectCameraPanel(int32 Number);
    void ToggleTravelSpeed() { const bool bOpen = IsTravelSpeedOpen(); CloseRadio(); if (!bOpen) RadioPage = 8; }
    void CloseRadio() { RadioPage = -1; CameraPage = ECameraPage::Closed; RadioRecipient = 0; RadioElementId = 0; RadioRecipientPage = 0; RadioHistory.Reset(); }
    void CycleMFD(int32 Index);
    bool CycleFlightHUDMode(bool bCurrentlyVisible);
    void ToggleWeaponsPanel() { bShowWeapons = !bShowWeapons; bLoggedPanelPaint = false; Invalidate(EInvalidateWidgetReason::Paint); }
    void ToggleCautionPanel() { bShowCaution = !bShowCaution; if (bShowCaution) bShowInformation = false; bLoggedPanelPaint = false; Invalidate(EInvalidateWidgetReason::Paint); }
    void ToggleInformationPanel() { bShowInformation = !bShowInformation; if (bShowInformation) bShowCaution = false; bLoggedPanelPaint = false; Invalidate(EInvalidateWidgetReason::Paint); }
    void CycleWeapon(bool bPrimary);
    static FString FormatWeaponReadout(const WeaponGroup* Group);
    static FString FormatStatusReadout(const FString& Label, double Percent, bool bAugmenter=false);
    bool IsCockpitDisplayActive() const;
private:
    TDelegate<bool(float)> OnOverviewWheel;
    friend class SCockpitInstrument;
    friend class FCockpitDisplaysTest;
    friend class FThirdPersonMiniHUDTest;
    TUniquePtr<FCockpitDisplays> CockpitDisplays;
    TUniquePtr<FNativeHUDSymbology> Symbology;
    double OrderedHeading=0,OrderedPitch=0;
    bool bHelmMode=false;
    FString CompactNavLabel=TEXT("FREE FLIGHT");
    void DrawILS(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
    Ship* GetSelectedRecoveryController(Ship& Player) const;
    void DrawFlightNavigation(Ship& Player, const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
    static TArray<FString> MissionStatusRows(const Ship* Player);
    void DrawSensorTracks(Ship& Player, const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
    friend class FRecoveryILSHUDTest;
    void UpdateILSMode(int32 PlayerId, bool bInbound);
    bool bILSMode = false;
    bool bILSInbound = false;
    int32 ILSPlayerId = INDEX_NONE;
    TStrongObjectPtr<UTexture2D> ILSTexture;
    FSlateBrush ILSBrush;
    int32 RadioPage = -1; // 0 root, 1 categories, 2 target, 3 combat, 4 formation, 5 mission, 6 sensors, 7 control
    enum class ECameraPage { Closed, Views, Look, Follow };
    ECameraPage CameraPage = ECameraPage::Closed;
    TWeakPtr<FMissionCameraRig> PanelCamera;
    void BuildCameraRows(TArray<FString>& Labels, TArray<int32>& Commands, FString& Title) const;
    TArray<int32> RadioHistory;
    int32 RadioElementId = 0;
    int32 RadioRecipientPage = 0;
    int32 RadioRecipient = 0; // 1 wingman, 2 element, 3 control
    struct FRow { FString Label; double Percent; };
    struct FBlip { double Az, El, Range; FLinearColor Color; FVector LocalDirection; };
    enum class EMode { Off, Ship, Mission, FOV, HSD, ThreeD };
    enum class EFlightHUDMode { Tactical, Navigation, ILS };
    EFlightHUDMode FlightHUDMode = EFlightHUDMode::Tactical;
    TAttribute<FString> CameraMode;
    void Refresh();
    void PaintRadarVolume(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer,
        const FVector2D& Center, double Radius, const FLinearColor& GridColor) const;
    void LoadShipHUDIcon(const FString& Path, FString& CachedPath, TStrongObjectPtr<UTexture2D>& Texture, FSlateBrush& Brush);
    TAttribute<bool> ShowHUD;
    TAttribute<bool> ThirdPerson;
    TWeakObjectPtr<ASSWSessionLobbyController> NetworkController;
    int32 NetworkEmcon = 0;
    TAttribute<TWeakObjectPtr<AActor>> SelectedTarget;
    TAttribute<FText> SelectedName;
    double NextSample = 0;
    mutable bool bLoggedPanelPaint = false;
    bool bShowWeapons = true;
    bool bShowCaution = false;
    bool bShowInformation = true;
    friend class FHUDInformationPanelTest;
    float InformationScale = 1.f;
    int32 PanelPlayerId = INDEX_NONE;
    TSharedPtr<SScrollBox> InformationScroll;
    TSharedPtr<SVerticalBox> InformationRows;
    TArray<FString> InformationCaptions;
    TArray<double> InformationExpiry;
    void UpdateInformationMessages();
    EMode Modes[2] = { EMode::Ship, EMode::FOV };
    TArray<FRow> StatusRows;
    TArray<SSWDamagePanel::FCell> DamageRows;
    TArray<FBlip> Contacts;
    FString PlayerName, PlayerClass, TargetName, TargetClass, Primary, Secondary;
    double TargetSpeed = -1;
    FString TargetRarity, TargetMineral, TargetDepositAmount, TargetDepositValue;
    FString TargetCargo;
    TArray<FString> JumpRoute;
    double PlayerShield = -1, PlayerHull = -1, TargetHull = -1, TargetShield = -1, TargetRange = -1, SensorRange = 1;
    bool bHasPlayer = false, bHasTarget = false;
    bool bAugmenterActive = false;
    FLinearColor TargetDisplayColor = FLinearColor::Yellow;
    FString HomeCarrierReadout;
    TArray<FString> NavReadouts, MissileReadouts;
    FString DefenseReadout;
    double FlightSpeed = 0, HeadingDegrees = 0, ClosingSpeed = 0, JumpSeconds = 0;
    double PitchDegrees = 0, BankRadians = 0;
    int32 ThreatLevel = 0;
    bool bAutoAvailable = false, bGearDown = false, bShoot = false;
    bool bTargetLocked = false;
    bool bTargetTracking = false;
    bool bClosingValid = false, bWarningFlash = false;
    FString PlayerIconPath, TargetIconPath;
    TMap<FString, TStrongObjectPtr<UTexture2D>> IconCache;
    TArray<FString> StatusTextLines;
    TStrongObjectPtr<UTexture2D> PlayerTexture, TargetTexture;
    FSlateBrush PlayerBrush, TargetBrush;
    TUniquePtr<FShipMFDPreview> PlayerMFDPreview;
    TUniquePtr<FShipMFDPreview> TargetMFDPreview;
    // Legacy-styled player launch button frame (UE-side texture only).
    TStrongObjectPtr<UTexture2D> LaunchTexture;
    FSlateBrush LaunchBrush;
    // TAC left/right, followed by sensor FOV/HSD/3D.
    TStrongObjectPtr<UTexture2D> PanelTextures[9];
    FSlateBrush PanelBrushes[9];
};
