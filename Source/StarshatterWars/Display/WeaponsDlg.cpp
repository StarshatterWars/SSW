#include "WeaponsDlg.h"
#include "WeaponsPopup.h"
#include "Engine/World.h"
TSharedRef<SWidget> UWeaponsDlg::CreatePanelContent(){
    const TWeakObjectPtr<UWeaponsDlg> Owner(this);
    return SNew(SWeaponsPopup).Embedded(true).FooterHost(FooterContent).FooterStyle(&FooterButtonStyle)
        .ResolveShip([Owner]()->Ship*{return Owner.IsValid()?Owner->ResolvePanelShip():nullptr;})
        .CanOperate([Owner](){return Owner.IsValid() && Owner->GetWorld() && !Owner->GetWorld()->IsPaused();})
        .OnClose(FSimpleDelegate::CreateWeakLambda(this,[this](){RequestPanelClose();}));
}
FText UWeaponsDlg::GetPanelCaption() const{return FText::FromString(TEXT("Weapons"));}
