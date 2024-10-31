// Copyright (c) 2023 Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "DebuffTypes.generated.h"

class ADebuffCharacter;
class UDebuffMovement;

namespace FDebuffTags
{
	PREDICTEDMOVEMENT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Debuff_Type_Snare);
}

/**
 * The method used to calculate debuff levels
 */
UENUM(BlueprintType)
enum class EDebuffLevelMethod : uint8
{
	Max				UMETA(ToolTip="The highest debuff level will be applied"),
	Stack			UMETA(ToolTip="Level stacks by each debuff that is applied. e.g. apply a level 1 debuff and a level 4 debuff, you reach level 5"),
	Average 		UMETA(ToolTip="The average debuff level will be applied"),
};

/**
 * FDebuffData
 * Represents a single debuff or buff that can be applied to a character
 */
USTRUCT(BlueprintType)
struct PREDICTEDMOVEMENT_API FDebuffData
{
	GENERATED_BODY()

	FDebuffData()
		: DebuffType(FDebuffTags::Debuff_Type_Snare)
		, LevelMethod(EDebuffLevelMethod::Max)
		, MaxDebuffs(3)
		, DebuffLevel(0)
	{}

	/** The type of debuff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debuff, meta=(Categories="Debuff.Type"))
	FGameplayTag DebuffType;

	/** The method used to calculate debuff levels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debuff)
	EDebuffLevelMethod LevelMethod;

	/** The maximum number of debuffs that can be applied at a single time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debuff)
	int32 MaxDebuffs;

public:
	/** The current debuff level */
	UPROPERTY()
	uint8 DebuffLevel;

	/** The debuff stack that is currently applied */
	UPROPERTY()
	TArray<uint8> Debuffs;

	UPROPERTY(Transient)
	TWeakObjectPtr<ADebuffCharacter> CharacterOwner;

public:
	template<typename T>
	T GetDebuffLevel() const
	{
		return static_cast<T>(DebuffLevel);
	}

	bool IsDebuffed() const
	{
		return DebuffLevel > 0;
	}

	int32 GetNumDebuffs() const	{ return Debuffs.Num();	}
	int32 GetNumDebuffsByLevel(uint8 Level = 0) const;

	void Initialize(ADebuffCharacter* InCharacterOwner);

	void Debuff(uint8 Level);
	void RemoveDebuff(uint8 Level);
	void RemoveAllDebuffs();
	void RemoveAllDebuffsByLevel(uint8 Level);
	void RemoveAllDebuffsExceptLevel(uint8 Level);
	
	void SetDebuffs(const TArray<uint8>& NewDebuffs);
	void SetDebuffLevel(uint8 Level);

	void OnDebuffsChanged();

public:
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	
	bool operator==(const FDebuffData& Other) const
	{
		return DebuffType == Other.DebuffType && DebuffLevel == Other.DebuffLevel && Debuffs == Other.Debuffs;
	}

	bool operator!=(const FDebuffData& Other) const
	{
		return !(*this == Other);
	}
};

template<>
struct TStructOpsTypeTraits<FDebuffData> : public TStructOpsTypeTraitsBase2<FDebuffData>
{
	enum
	{
		WithNetSerializer = true
	};
};

// struct PREDICTEDMOVEMENT_API FDebuffPredictionData
// {
// 	FDebuffPredictionData()
// 		: DebuffLevel(0)
// 	{}
// 	
// 	uint8 DebuffLevel;
// 	TArray<uint8> Debuffs;
//
// 	bool operator==(const FDebuffPredictionData& Other) const
// 	{
// 		return DebuffLevel == Other.DebuffLevel && Debuffs == Other.Debuffs;
// 	}
//
// 	bool operator!=(const FDebuffPredictionData& Other) const
// 	{
// 		return !(*this == Other);
// 	}
//
// 	bool operator==(const FDebuffData& Other) const
// 	{
// 		return DebuffLevel == Other.DebuffLevel && Debuffs == Other.Debuffs;
// 	}
//
// 	bool operator!=(const FDebuffData& Other) const
// 	{
// 		return !(*this == Other);
// 	}
//
// 	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
// 	{
// 		SerializeOptionalValue<uint8>(Ar.IsSaving(), Ar, DebuffLevel, 0);
//
// 		
//
// 		Ar << Debuffs;
// 		return !Ar.IsError();
// 	}
// };
//
// template<>
// struct TStructOpsTypeTraits<FDebuffPredictionData> : public TStructOpsTypeTraitsBase2<FDebuffPredictionData>
// {
// 	enum
// 	{
// 		WithNetSerializer = true
// 	};
// };