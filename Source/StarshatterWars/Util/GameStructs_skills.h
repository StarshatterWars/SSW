#pragma once
#include "CoreMinimal.h"
#include "GameStructs_skills.generated.h"

USTRUCT(BlueprintType)
struct FSkillDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SkillTypeID = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SkillRank = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName PrimaryAttribute;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SecondaryAttribute;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bIsPublished = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bIsAvailableForPurchase = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Group;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 BookPrice = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 AdvancedBookPrice = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MinimumRank = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Empire;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Prerequisites;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int32> RequiredSkillLevels;
    // All skills share the level-duration schedule from training.json.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaximumLevel = 10;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double TrainingMultiplier = 1;
};
USTRUCT(BlueprintType)
struct FSkillProgress
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid Id;
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bComplete = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bAdvancedBookOwned = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) int64 LearnedUtc = 0;
    UPROPERTY(SaveGame) int32 TrainedLevel = 0;
    UPROPERTY(SaveGame) double SkillPoints = 0;
    UPROPERTY(SaveGame) int32 TrainingTargetLevel = 0;
    UPROPERTY(SaveGame) int64 TrainingStartedUtc = 0;
    UPROPERTY(SaveGame) int64 TrainingFinishesUtc = 0;
};

USTRUCT()
struct FSkillBookOffer
{
    GENERATED_BODY()
    UPROPERTY() int32 SkillTypeID = 0;
    UPROPERTY() FString Name;
    UPROPERTY() int64 BasePrice = 0;
    UPROPERTY() double PurchaseTaxRate = 0;
    UPROPERTY() bool IsPublished = true;
    UPROPERTY() bool IsAvailableForPurchase = true;
    UPROPERTY() bool GrantCompleteSkill = false;
};

USTRUCT(BlueprintType)
struct FSkillTrainingEntry
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Skill;
    UPROPERTY(SaveGame) int32 TargetLevel=1;
    UPROPERTY(SaveGame) int64 StartedUtc=0;
    UPROPERTY(SaveGame) int64 FinishesUtc=0;
};
