#pragma once
#include "Engine/Engine.h"
#include "Engine/World.h"
struct FScopedSSWTestWorldContext
{
 explicit FScopedSSWTestWorldContext(UWorld* InWorld):World(InWorld)
 {
  if(GEngine && World && !GEngine->GetWorldContextFromWorld(World))
  {GEngine->CreateNewWorldContext(World->WorldType).SetCurrentWorld(World);bOwned=true;}
 }
 ~FScopedSSWTestWorldContext(){if(bOwned && GEngine && GEngine->GetWorldContextFromWorld(World))GEngine->DestroyWorldContext(World);}
 FScopedSSWTestWorldContext(const FScopedSSWTestWorldContext&)=delete;
 FScopedSSWTestWorldContext& operator=(const FScopedSSWTestWorldContext&)=delete;
 UWorld* World=nullptr;bool bOwned=false;
};
