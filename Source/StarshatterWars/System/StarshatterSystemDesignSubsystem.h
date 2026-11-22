#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs_System.h"
#include "StarshatterSystemDesignSubsystem.generated.h"

USTRUCT()
struct FSystemDesignDocument
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSystemComponentGrade> ComponentGrades;

    UPROPERTY()
    TArray<FSystemDesign> Systems;
};

UCLASS()
class STARSHATTERWARS_API UStarshatterSystemDesignSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemDesign")
    void LoadSystemDesigns();

    static bool ImportDefinitions(UDataTable& Table);
    static bool ReadJsonCatalog(const FString& Path,TMap<FName,FSystemDesign>& Out,FString& Error);
    bool ReloadJsonCatalog(const FString& Path,FString& Error);
    void LoadAll(bool bImportJson);

    const TMap<FName, FSystemDesign>& GetDesignsByName() const { return DesignsByName; }
    const FSystemDesign* FindDesign(FName Name) const { return DesignsByName.Find(Name); }
    TOptional<FComponentDesign> FindComponentGrade(FName System, FName Component, int32 Grade) const;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Starshatter|SystemDesign")
    TObjectPtr<UDataTable> SystemDesignDataTable;

private:
    UPROPERTY()
    TMap<FName, FSystemDesign> DesignsByName;
};