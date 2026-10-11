#include "ContactsDlg.h"
#include "WeaponsPopup.h"
#include "Engine/World.h"
TSharedRef<SWidget> UContactsDlg::CreatePanelContent(){
    const TWeakObjectPtr<UContactsDlg> Owner(this);
    return SNew(SWeaponsPopup).Embedded(true).ContactsOnly(true).FooterHost(FooterContent).FooterStyle(&FooterButtonStyle)
        .ResolveShip([Owner]()->Ship*{return Owner.IsValid()?Owner->ResolvePanelShip():nullptr;})
        .CanOperate([Owner](){return Owner.IsValid() && Owner->GetWorld() && !Owner->GetWorld()->IsPaused();})
        .OnClose(FSimpleDelegate::CreateWeakLambda(this,[this](){RequestPanelClose();}));
}
FText UContactsDlg::GetPanelCaption() const{return FText::FromString(TEXT("Contacts"));}
