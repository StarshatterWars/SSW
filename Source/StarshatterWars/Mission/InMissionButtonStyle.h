#pragma once
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
// Panel-owned resources: brushes outlive every Slate widget referring to them.
struct FInMissionButtonStyle {
 FButtonStyle Cyan,Red,Green;
 TArray<TStrongObjectPtr<UTexture2D>> Textures;
 TMap<FString,TSharedPtr<FSlateBrush>> Icons;
 FInMissionButtonStyle(){Cyan=Load(TEXT("Blue"));Red=Load(TEXT("Red"));Green=Load(TEXT("Green"));}
 FButtonStyle Load(const TCHAR* Color){auto S=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");auto B=[&](const TCHAR* State){FSlateBrush Brush;const FString P=FString::Printf(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01%s_%s.T_Btn01%s_%s"),Color,State,Color,State);if(auto* T=LoadObject<UTexture2D>(nullptr,*P)){Textures.Emplace(T);Brush.SetResourceObject(T);Brush.DrawAs=ESlateBrushDrawType::Image;}return Brush;};S.Normal=B(TEXT("n"));S.Hovered=B(TEXT("p"));S.Pressed=S.Hovered;return S;}
 TSharedRef<SWidget> Content(const FString& Caption,int Size=14){
  const FString Key=Caption.ToUpper();FString Icon;
  if(Key==TEXT("CLOSE"))Icon=TEXT("Check");
  if(!Icon.IsEmpty()){
   if(!Icons.Contains(Icon)){const FString N=TEXT("T_")+Icon+TEXT("_256x256"),P=TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/")+N+TEXT(".")+N;auto Brush=MakeShared<FSlateBrush>();if(auto* T=LoadObject<UTexture2D>(nullptr,*P)){Textures.Emplace(T);Brush->SetResourceObject(T);Brush->DrawAs=ESlateBrushDrawType::Image;}Icons.Add(Icon,Brush);}
   return SNew(SBox).WidthOverride(28).HeightOverride(28).ToolTipText(FText::FromString(Caption))[SNew(SImage).Image(Icons[Icon].Get()).ColorAndOpacity(FLinearColor::White)];
  }
  return SNew(STextBlock).Text(FText::FromString(Caption)).ToolTipText(FText::FromString(Caption)).ColorAndOpacity(FLinearColor::White).Font(FCoreStyle::GetDefaultFontStyle("Bold",Size)).AutoWrapText(true);
 }
};
