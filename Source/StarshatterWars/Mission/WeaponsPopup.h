#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Ship.h"
#include "WeaponGroup.h"
#include "Weapon.h"

// Legacy WepView interaction, hosted independently of the panel registry.
// The host supplies current simulation ownership, availability, and close behavior.
// Never capture a simulation pointer in a Slate callback.
class SWeaponsPopup : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWeaponsPopup) {}
        SLATE_ARGUMENT(TFunction<Ship*()>, ResolveShip)
        SLATE_ARGUMENT(TFunction<bool()>, CanOperate)
        SLATE_EVENT(FSimpleDelegate, OnClose)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        ResolveShip=Args._ResolveShip;
        CanOperate=Args._CanOperate;
        Close=Args._OnClose;
        const TCHAR* Names[]={TEXT("TAC_left"),TEXT("TAC_right"),TEXT("TAC_button"),TEXT("MAN"),TEXT("AUTO"),TEXT("DEF")};
        for (int32 I=0; I<6; ++I) {
            const FString Path=FString::Printf(TEXT("/Game/UI/HUD/%s.%s"),Names[I],Names[I]);
            UTexture2D* Texture=LoadObject<UTexture2D>(nullptr,*Path);
            Textures[I].Reset(Texture);
            if (Texture) {
                Brushes[I].SetResourceObject(Texture);
                Brushes[I].ImageSize=FVector2D(Texture->GetSizeX(),Texture->GetSizeY());
                Brushes[I].DrawAs=ESlateBrushDrawType::Image;
            }
        }
        ChildSlot
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
         .BorderBackgroundColor(FLinearColor(0,0,0,0.25f)).Padding(20)
         .HAlign(HAlign_Center).VAlign(VAlign_Top)
         [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
          [SNew(SBox).WidthOverride(1024).HeightOverride(350)
           [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.005f,0.02f,0.035f,0.95f)).Padding(8)
            [SNew(SOverlay)
             +SOverlay::Slot().VAlign(VAlign_Top)
              [SNew(SBox).HeightOverride(180)
               [SNew(SHorizontalBox)
                +SHorizontalBox::Slot()[SNew(SImage).Image(&Brushes[0]).ColorAndOpacity(Blue())]
                +SHorizontalBox::Slot()[SNew(SImage).Image(&Brushes[1]).ColorAndOpacity(Blue())]]]
             +SOverlay::Slot()[SAssignNew(Root,SVerticalBox)]]]]]];
        Rebuild();
    }

    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& Event) override
    {
        if (Event.GetKey()==EKeys::Escape || Event.GetKey()==EKeys::Delete) {
            StopFire(); Close.ExecuteIfBound(); return FReply::Handled();
        }
        return FReply::Unhandled(); // IA_WeaponsPanel remains owned by the host.
    }
    virtual void OnFocusLost(const FFocusEvent& Event) override
    {
        StopFire(); SCompoundWidget::OnFocusLost(Event);
    }
    virtual void Tick(const FGeometry& Geometry,double Time,float Delta) override
    {
        SCompoundWidget::Tick(Geometry,Time,Delta);
        Ship* P=Player();
        FString NewSignature=FString::Printf(TEXT("%p"),static_cast<void*>(P));
        if (P) for (int32 I=0; I<P->GetWeapons().size(); ++I)
            NewSignature+=FString::Printf(TEXT(";%p"),static_cast<void*>(P->GetWeapons()[I]));
        if (NewSignature!=Signature) {
            Signature=NewSignature; StopFire(); Page=0; Rebuild();
        }
        if (!Operable() || !FSlateApplication::Get().IsActive() ||
            !FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton)) StopFire();
        if (HeldGroup>=0 && Group(HeldGroup) && P) P->FireWeapon(HeldGroup);
    }

private:
    TFunction<Ship*()> ResolveShip;
    TFunction<bool()> CanOperate;
    FSimpleDelegate Close;
    TSharedPtr<SVerticalBox> Root;
    TStrongObjectPtr<UTexture2D> Textures[6];
    FSlateBrush Brushes[6];
    FString Signature;
    int32 Page=0;
    int32 HeldGroup=INDEX_NONE;

    static FLinearColor Blue() { return FLinearColor(0.25f,0.7f,1.0f); }
    static FText Text(const FString& S) { return FText::FromString(S); }
    Ship* Player() const { return ResolveShip ? ResolveShip() : nullptr; }
    bool Operable() const { return Player() && (!CanOperate || CanOperate()); }
    WeaponGroup* Group(int32 I) const {
        Ship* P=Player();
        return P && I>=0 && I<P->GetWeapons().size() ? P->GetWeapons()[I] : nullptr;
    }
    void StopFire() { HeldGroup=INDEX_NONE; }
    TSharedRef<SWidget> Label(const FString& S,int32 Size=16) {
        return SNew(STextBlock).Text(Text(S)).ColorAndOpacity(Blue())
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",Size));
    }
    TSharedRef<SWidget> OrderButton(int32 I,WeaponsOrders Order,int32 Art,const TCHAR* Caption) {
        return SNew(SButton).IsFocusable(false)
            .ToolTipText(Text(Caption))
            .IsEnabled_Lambda([this,I](){return Operable() && Group(I);})
            .OnClicked_Lambda([this,I,Order](){if (Operable()) if (auto* G=Group(I)) G->SetFiringOrders(Order); return FReply::Handled();})
            [SNew(SBox).HeightOverride(22)
             [SNew(SOverlay)
              +SOverlay::Slot()[SNew(SImage).Image(&Brushes[Art])
                .ColorAndOpacity_Lambda([this,I,Order](){auto* G=Group(I);return G && G->GetFiringOrders()==Order ? Blue() : FLinearColor(0.12f,0.2f,0.25f);})]
              +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                [SNew(STextBlock).Text(Text(Caption))
                 .Visibility(Textures[Art].IsValid()?EVisibility::Collapsed:EVisibility::Visible)
                 .ColorAndOpacity(Blue())]]];
    }
    FString Readout(int32 I) const {
        auto* G=Group(I); if (!G) return TEXT("Unavailable");
        int32 Ammo=0, Powered=0, Count=0; bool Unlimited=false;
        double Charge=0;
        for (int32 W=0; W<G->NumWeapons(); ++W) if (auto* Gun=G->GetWeapon(W)) {
            if (Gun->Ammo()<0) Unlimited=true; else Ammo+=Gun->Ammo();
            Powered+=Gun->IsPowerOn()?1:0;
            if (Gun->GetCapacity()>0) Charge+=FMath::Clamp(double(Gun->Charge()),0.0,100.0);
            ++Count;
        }
        const FString Amount=Unlimited ? TEXT("UNLIMITED") : FString::FromInt(Ammo);
        return FString::Printf(TEXT("AMMO  %s\nCHARGE  %.0f%%\nPOWER  %d / %d"),*Amount,Count?Charge/Count:0.0,Powered,Count);
    }
    void Rebuild() {
        Root->ClearChildren();
        Root->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,4,0,18)[Label(TEXT("TACTICAL WEAPONS"),20)];
        TSharedPtr<SHorizontalBox> Columns;
        Root->AddSlot().FillHeight(1)[SAssignNew(Columns,SHorizontalBox)];
        Ship* P=Player(); const int32 Count=P?P->GetWeapons().size():0;
        if (!Count) Columns->AddSlot().HAlign(HAlign_Center)[Label(TEXT("No weapon groups available."))];
        for (int32 Slot=0; Slot<4 && Page*4+Slot<Count; ++Slot) {
            const int32 I=Page*4+Slot;
            auto* G=Group(I); if (!G) continue;
            Columns->AddSlot().FillWidth(1).Padding(8)
            [SNew(SVerticalBox)
             +SVerticalBox::Slot().AutoHeight()
              [SNew(SButton).IsFocusable(false).ToolTipText(Text(TEXT("Hold to fire this weapon group (legacy TAC control).")))
               .IsEnabled_Lambda([this,I](){auto* Current=Group(I); return Operable() && Current && Current->NumWeapons()>0 && Current->GetFiringOrders()==WeaponsOrders::MANUAL;})
               .OnPressed_Lambda([this,I](){if (Operable()) HeldGroup=I;})
               .OnReleased_Lambda([this](){StopFire();})
               [SNew(SBox).HeightOverride(36)
                [SNew(SOverlay)
                 +SOverlay::Slot()[SNew(SImage).Image(&Brushes[2]).ColorAndOpacity(Blue())]
                 +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Label(ANSI_TO_TCHAR(G->Name()))]]]]
             +SVerticalBox::Slot().AutoHeight().Padding(0,6)
              [SNew(SHorizontalBox)
               +SHorizontalBox::Slot()[OrderButton(I,WeaponsOrders::MANUAL,3,TEXT("MAN"))]
               +SHorizontalBox::Slot()[OrderButton(I,WeaponsOrders::AUTO,4,TEXT("AUTO"))]
               +SHorizontalBox::Slot()[OrderButton(I,WeaponsOrders::POINT_DEFENSE,5,TEXT("DEF"))]]
             +SVerticalBox::Slot().AutoHeight().Padding(4,14)
              [SNew(STextBlock).ColorAndOpacity(Blue()).Font(FCoreStyle::GetDefaultFontStyle("Regular",16))
               .Text_Lambda([this,I](){return Text(Readout(I));})]];
        }
        Root->AddSlot().AutoHeight().Padding(8)
        [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",16))
         .Text_Lambda([this](){auto* Current=Player(); if (!Current || !Current->GetTarget())return Text(TEXT("NO TARGET"));
             auto* Sub=Current->GetSubTarget();return Text(FString(ANSI_TO_TCHAR(Sub?Sub->Abbreviation():Current->GetTarget()->GetName())).ToUpper());})
         .ColorAndOpacity_Lambda([this](){auto* Current=Player();auto* Sub=Current?Current->GetSubTarget():nullptr;
             if (!Sub) return Blue();
             if (Sub->GetStatus()==SYSTEM_STATUS::DESTROYED || Sub->GetStatus()==SYSTEM_STATUS::CRITICAL)return FLinearColor::Red;
             if (Sub->GetStatus()==SYSTEM_STATUS::DEGRADED)return FLinearColor::Yellow;
             if (Sub->GetStatus()==SYSTEM_STATUS::MAINT && FMath::Fmod(FPlatformTime::Seconds(),0.5)<0.25)return FLinearColor(0.03f,0.03f,0.03f);
             return Blue();})];
        Root->AddSlot().AutoHeight()
        [SNew(SHorizontalBox)
         +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).IsEnabled(Page>0).OnClicked_Lambda([this](){StopFire();--Page;Rebuild();return FReply::Handled();})[Label(TEXT("Previous"))]]
         +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center)[Label(FString::Printf(TEXT("%d / %d"),Page+1,FMath::Max(1,(Count+3)/4))) ]
         +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[SNew(SButton).IsEnabled((Page+1)*4<Count).OnClicked_Lambda([this](){StopFire();++Page;Rebuild();return FReply::Handled();})[Label(TEXT("Next"))]]
         +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).OnClicked_Lambda([this](){StopFire();Close.ExecuteIfBound();return FReply::Handled();})[Label(TEXT("Close"))]]];
    }
};
