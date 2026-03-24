// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_Galaxy struct
#include "SSWGameInstance.h"
#include "GalaxyMap.generated.h"

class UCanvasPanel;
class USystemMarker;
class UGalaxyLink;
class UJumpLinksWidget;
class UMapGridLine;
class USelectionLinesWidget;
class UOperationsScreen;
class UTexture2D;

/**
 * 
 */

 USTRUCT()
struct FGalaxyLinkKey
{
    GENERATED_BODY()

    FString A;
    FString B;

    FGalaxyLinkKey() {}
    FGalaxyLinkKey(const FString& InA, const FString& InB)
    {
        if (InA < InB)
        {
            A = InA;
            B = InB;
        }
        else
        {
            A = InB;
            B = InA;
        }
	}

	bool operator==(const FGalaxyLinkKey& Other) const
	{
		return A == Other.A && B == Other.B;
	}

	friend uint32 GetTypeHash(const FGalaxyLinkKey& Key)
	{
		return HashCombine(GetTypeHash(Key.A), GetTypeHash(Key.B));
	}
};

class UOperationsScreen;

UCLASS()
class STARSHATTERWARS_API UGalaxyMap : public UUserWidget
{
	GENERATED_BODY()
	
protected:
    void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void NativeOnInitialized() override; 
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    void PanToMarker(const FVector2D& MarkerCenter);
    void UpdateCameraPan();

    UPROPERTY()
	UOperationsScreen* Owner = nullptr;

	UPROPERTY()
	FString CurrentSystemName;

	UPROPERTY()
	class USystemMarker* SelectedMarker = nullptr;

public:
    // TSubclassOf must be set in UMG (or via C++)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TSubclassOf<USystemMarker> MarkerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TSubclassOf<UGalaxyLink> GalaxyLink; // Optional visual line widget

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TSubclassOf<class UJumpLinksWidget> JumpLinksWidgetClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TSubclassOf<class USelectionLinesWidget> SelectionLinesWidgetClass;

    UPROPERTY(EditAnywhere, Category = "Galaxy")
    TSubclassOf<class UMapGridLine> MapGridLines;

    UPROPERTY()
    class USelectionLinesWidget* SelectionLinesWidget = nullptr;

    UPROPERTY()
    class UJumpLinksWidget* JumpLinksWidget = nullptr;

    UPROPERTY()
    class UMapGridLine* GalaxyGridWidget;

    UPROPERTY()
    TArray<FJumpLink> JumpLinks;

    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* MapRoot; // top-level canvas in WBP_GalaxyMap 
    
    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* MapCanvas;  // contains markers, grid, links

     UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* MapCameraRoot; // wraps MapCanvas, handles pan/zoom

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TMap<FName, UTexture2D*> StarTextures;

    UPROPERTY()
    TArray<FS_Galaxy> GalaxySystems; // Store this to access system bounds

    void SetOwner(UOperationsScreen* InOwner) { Owner = InOwner; }

    UPROPERTY()
    float MapScale = 70.0f;
    
    UPROPERTY()
    float PanDuration = 0.3f;
    
    UPROPERTY()
    float PanElapsed = 0.f;

    // Zoom and pan state
    UPROPERTY()
    float MapZoomLevel = 1.0f;
    const float MinZoom = 0.5f;
    const float MaxZoom = 1.5f;

    void BuildGalaxyMap(const TArray<FS_Galaxy>& Systems);
    
    TMap<FName, UTexture2D*> LoadStarTextures();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D ScreenOffset;

private:
    
    UFUNCTION()
    void DrawLinkBetween(const FS_Galaxy& A, const FS_Galaxy& B);
    UFUNCTION()
    FVector2D ProjectTo2D(const FVector& Location) const;
    UFUNCTION()
    FVector2D LineProjectTo2D(const FVector& Location) const;
    
    TMap<FString, FS_Galaxy> SystemLookup;
    TMap<FString, USystemMarker*> MarkerMap;
    TSet<FGalaxyLinkKey> DrawnLinks;   

	FTimerHandle CameraPanHandle;
	FVector2D PanStartOffset;
	FVector2D PanTargetOffset;
    FVector2D CurrentPan = FVector2D::ZeroVector;

    // For dragging
    bool bIsPanning = false;
    FVector2D PanStartMouse;
};
	

