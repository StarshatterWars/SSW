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
#include "SimContact.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"

// Legacy WepView interaction, hosted independently of the panel registry.
// The host supplies current simulation ownership, availability, and close behavior.
// Never capture a simulation pointer in a Slate callback.
class SWeaponsPopup : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWeaponsPopup) : _Embedded(false), _FooterStyle(nullptr) {}
        SLATE_ARGUMENT(bool, Embedded)
        SLATE_ARGUMENT(TSharedPtr<SBox>, FooterHost)
        SLATE_ARGUMENT(const FButtonStyle*, FooterStyle)
        SLATE_ARGUMENT(TFunction<Ship*()>, ResolveShip)
        SLATE_ARGUMENT(TFunction<bool()>, CanOperate)
        SLATE_EVENT(FSimpleDelegate, OnClose)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        Embedded=Args._Embedded;
        FooterHost=Args._FooterHost;
        FooterStyle=Args._FooterStyle;
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
        if (Embedded)
            ChildSlot[SAssignNew(Root,SVerticalBox)];
        else
            ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                [SNew(SBox).WidthOverride(1024).HeightOverride(1024)
                    [SAssignNew(Root,SVerticalBox)]]];
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
        if(Time>=NextContactRefresh){NextContactRefresh=Time+0.5;RefreshContacts();}
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
    bool Embedded=false;
    TSharedPtr<SBox> FooterHost;
    const FButtonStyle* FooterStyle=nullptr;
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
    TSharedRef<SWidget> ButtonLabel(const FString& S,int32 Size=16) {
        return SNew(STextBlock).Text(Text(S.ToUpper())).ColorAndOpacity(FLinearColor::Black)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",Size)).AutoWrapText(true);
    }
    TSharedRef<SWidget> OrderButton(int32 I,WeaponsOrders Order,const TCHAR* Caption) {
        return SNew(SButton).IsFocusable(false)
            .ToolTipText(Text(Caption))
            .ButtonColorAndOpacity_Lambda([this,I,Order](){auto* G=Group(I);return G && G->GetFiringOrders()==Order ? FLinearColor(0.45f,0.8f,1.0f) : FLinearColor::White;})
            .IsEnabled_Lambda([this,I](){return Operable() && Group(I);})
            .OnClicked_Lambda([this,I,Order](){if (Operable()) if (auto* G=Group(I)) G->SetFiringOrders(Order); return FReply::Handled();})
            [ButtonLabel(Caption,14)];
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
    Ship* ContactShip(Ship* Identity) const {
        auto* P=Player(); if (!P) return nullptr;
        auto& Contacts=P->GetContactList();
        for (int32 I=0; I<Contacts.size(); ++I) {
            auto* C=Contacts[I];
            if (C && C->GetShip()==Identity && C->Visible(P))
                return Identity && !Identity->IsDead() ? Identity : nullptr;
        }
        return nullptr;
    }
    TSharedRef<SWidget> Frame(TSharedRef<SWidget> Content) {
        return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor((Embedded?FLinearColor::Transparent:FLinearColor(0.01f,0.025f,0.04f,0.94f))).Padding(8)[Content];
    }
    void RefreshContacts() {
        if (!ContactsBox.IsValid()) return;
        auto* P=Player();
        FString State=FString::Printf(TEXT("%p"),static_cast<void*>(P));
        if(P){
            State+=FString::Printf(TEXT(":%p:%p"),static_cast<void*>(P->GetTarget()),static_cast<void*>(P->GetSubTarget()));
            auto& List=P->GetContactList();
            for(int32 J=0;J<List.size();++J)if(auto* C=List[J])
                if(C->Visible(P) && C->GetShip())State+=FString::Printf(TEXT(";%p"),static_cast<void*>(C->GetShip()));
            if(auto* T=dynamic_cast<Ship*>(P->GetTarget()))for(int32 J=0;J<T->GetSystems().size();++J)
                if(auto* Sys=T->GetSystems()[J])State+=FString::Printf(TEXT(";%p:%.0f"),static_cast<void*>(Sys),Sys->GetAvailability());
        }
        if(State==ContactState)return;
        ContactState=State;
        ContactsBox->ClearChildren(); SystemsBox->ClearChildren();
        if (!P) return;
        auto& Contacts=P->GetContactList();
        for (int32 I=0; I<Contacts.size(); ++I) {
            auto* C=Contacts[I]; auto* T=C?C->GetShip():nullptr;
            if (!T || T==P || !C->Visible(P) || T->IsDead()) continue;
            ContactsBox->AddSlot().AutoHeight().Padding(0,2)
            [SNew(SButton).IsFocusable(false)
             .OnClicked_Lambda([this,T](){if (Operable()) if (auto* Current=ContactShip(T)) Player()->SetTarget(Current);RefreshContacts();return FReply::Handled();})
             [ButtonLabel(FString::Printf(TEXT("%s%hs"),P->GetTarget()==T?TEXT("> "):TEXT(""),T->GetName()),14)]];
        }
        auto* Target=dynamic_cast<Ship*>(P->GetTarget());
        if (!Target) {SystemsBox->AddSlot().AutoHeight()[Label(TEXT("Select a ship contact."),14)];return;}
        for (int32 I=0; I<Target->GetSystems().size(); ++I) {
            auto* System=Target->GetSystems()[I]; if (!System) continue;
            SystemsBox->AddSlot().AutoHeight().Padding(0,2)
            [SNew(SButton).IsFocusable(false)
             .OnClicked_Lambda([this,Target,System](){
                auto* Current=ContactShip(Target);
                if (Operable() && Current && Player()->GetTarget()==Current) {
                    for (int32 J=0;J<Current->GetSystems().size();++J)
                        if (Current->GetSystems()[J]==System) {Player()->SetTarget(Current,System);break;}
                }
                RefreshContacts();return FReply::Handled();})
             [ButtonLabel(FString::Printf(TEXT("%s%hs  %.0f%%"),P->GetSubTarget()==System?TEXT("> "):TEXT(""),System->GetName(),System->GetAvailability()),14)]];
        }
    }
    void Rebuild() {
        Root->ClearChildren();
        if(!Embedded) Root->AddSlot().AutoHeight()[Frame(Label(TEXT("TACTICAL WEAPONS"),20))];
        TSharedPtr<SVerticalBox> Rows;
        ContactState.Empty();
        ContactsBox=SNew(SVerticalBox); SystemsBox=SNew(SVerticalBox);
        auto ContactScroll=SNew(SScrollBox)+SScrollBox::Slot()[ContactsBox.ToSharedRef()];
        auto SystemScroll=SNew(SScrollBox)+SScrollBox::Slot()[SystemsBox.ToSharedRef()];
        auto ContactsPane=SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[Label(TEXT("CONTACT LIST"))]
            +SVerticalBox::Slot().FillHeight(1)[ContactScroll];
        auto SystemsPane=SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[Label(TEXT("TARGET SYSTEMS"))]
            +SVerticalBox::Slot().FillHeight(1)[SystemScroll];
        Root->AddSlot().AutoHeight().Padding(8,0,8,8)[Label(TEXT("SHIP WEAPONS"),18)];
        Root->AddSlot().FillHeight(0.46f).Padding(0,0,0,16)
            [Frame(SAssignNew(Rows,SVerticalBox))];
        auto ContactLists=SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1).Padding(4)[ContactsPane]
            +SHorizontalBox::Slot().FillWidth(1).Padding(4)[SystemsPane];
        Root->AddSlot().FillHeight(0.54f)[Frame(ContactLists)];
        auto* P=Player(); const int32 Count=P?P->GetWeapons().size():0;
        if (!Count) Rows->AddSlot().AutoHeight()[Label(TEXT("No weapon groups."))];
        for (int32 Slot=0;Slot<4 && Page*4+Slot<Count;++Slot) {
            const int32 I=Page*4+Slot;auto* G=Group(I);if(!G)continue;
            Rows->AddSlot().FillHeight(1).Padding(0,3)
            [SNew(SHorizontalBox)
             +SHorizontalBox::Slot().FillWidth(1.5f)
              [SNew(SButton).IsFocusable(false).ToolTipText(Text(TEXT("Hold to request group fire")))
               .IsEnabled_Lambda([this,I](){auto* W=Group(I);return Operable() && W && W->NumWeapons()>0 && W->GetFiringOrders()==WeaponsOrders::MANUAL;})
               .OnPressed_Lambda([this,I](){if(Operable())HeldGroup=I;})
               .OnReleased_Lambda([this](){StopFire();})
               [ButtonLabel(ANSI_TO_TCHAR(G->Name()),14)]]
             +SHorizontalBox::Slot().AutoWidth().Padding(3)[SNew(SBox).WidthOverride(12)
              [SNew(SProgressBar).BarFillType(EProgressBarFillType::BottomToTop)
               .Percent_Lambda([this,I](){auto* W=Group(I);double Sum=0;int32 N=0;if(W)for(int32 J=0;J<W->NumWeapons();++J)if(auto* Gun=W->GetWeapon(J)){if(Gun->GetCapacity()>0){Sum+=Gun->Charge();++N;}}
                   return TOptional<float>(N?FMath::Clamp(float(Sum/N/100),0.0f,1.0f):0.0f);})]]
             +SHorizontalBox::Slot().FillWidth(0.65f)[OrderButton(I,WeaponsOrders::MANUAL,TEXT("MAN"))]
             +SHorizontalBox::Slot().FillWidth(0.65f)[OrderButton(I,WeaponsOrders::AUTO,TEXT("AUTO"))]
             +SHorizontalBox::Slot().FillWidth(0.65f)[OrderButton(I,WeaponsOrders::POINT_DEFENSE,TEXT("DEF"))]
             +SHorizontalBox::Slot().FillWidth(1).Padding(5,0)
              [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(Blue()).Text_Lambda([this,I](){return Text(Readout(I));})]];
        }
        auto Footer=SNew(SHorizontalBox)
          +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FooterStyle ? FooterStyle : &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button")).ContentPadding(FMargin(24,10)).IsEnabled(Page>0).OnClicked_Lambda([this](){StopFire();--Page;Rebuild();return FReply::Handled();})[ButtonLabel(TEXT("Previous"))]]
          +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center)[Label(FString::Printf(TEXT("%d / %d"),Page+1,FMath::Max(1,(Count+3)/4)))]
          +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton).ButtonStyle(FooterStyle ? FooterStyle : &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button")).ContentPadding(FMargin(24,10)).IsEnabled((Page+1)*4<Count).OnClicked_Lambda([this](){StopFire();++Page;Rebuild();return FReply::Handled();})[ButtonLabel(TEXT("Next"))]]
          +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FooterStyle ? FooterStyle : &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button")).ContentPadding(FMargin(24,10)).OnClicked_Lambda([this](){StopFire();Close.ExecuteIfBound();return FReply::Handled();})[ButtonLabel(TEXT("Close"))]];
        if (FooterHost.IsValid()) FooterHost->SetContent(Footer);
        else Root->AddSlot().AutoHeight()[Frame(Footer)];
        RefreshContacts();
    }
    TSharedPtr<SVerticalBox> ContactsBox, SystemsBox;
    double NextContactRefresh=0;
    FString ContactState;
};
