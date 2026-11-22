#pragma once
#include "CoreMinimal.h"
// Presentation state only. Authority snapshots/events supply revisions; widgets never mutate game state.
class FPanelAttentionState
{
public:
 void BeginSession(const FString& Id){if(Session!=Id){Session=Id;Revisions.Reset();Unread.Reset();}}
 void Observe(FName Panel,uint32 Revision,bool bOpen)
 {
  if(const auto* Prior=Revisions.Find(Panel);Prior && *Prior!=Revision && !bOpen)Unread.Add(Panel);
  Revisions.Add(Panel,Revision);if(bOpen)Unread.Remove(Panel);
 }
 void Acknowledge(FName Panel){Unread.Remove(Panel);}
 bool IsUnread(FName Panel)const{return Unread.Contains(Panel);}
private:
 FString Session;TMap<FName,uint32> Revisions;TSet<FName> Unread;
};
