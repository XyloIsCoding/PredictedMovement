// Copyright (c) 2023 Jared Taylor. All Rights Reserved.


#include "Debuff/DebuffTypes.h"

#include "Algo/Accumulate.h"
#include "Algo/MaxElement.h"
#include "Debuff/DebuffCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DebuffTypes)

namespace FDebuffTags
{
	UE_DEFINE_GAMEPLAY_TAG(Debuff_Type_Snare, "Debuff.Type.Snare");
}

int32 FDebuffData::GetNumDebuffsByLevel(uint8 Level) const
{
	// Can't apply a debuff of level 0 in the first place
	if (!ensureAlways(Level > 0))
	{
		return 0;
	}
	
	return Debuffs.FilterByPredicate([Level](uint8 DebuffLevel)
	{
		return DebuffLevel == Level;
	}).Num();
}

void FDebuffData::Initialize(ADebuffCharacter* InCharacterOwner)
{
	CharacterOwner = InCharacterOwner;
}

void FDebuffData::Debuff(uint8 Level)
{
	if (!ensureAlwaysMsgf(Level > 0, TEXT("Cannot apply a debuff of level 0")))
	{
		return;
	}

	if (Debuffs.Num() == MaxDebuffs)
	{
		Debuffs.RemoveAt(0);
	}

	Debuffs.Add(Level);
	OnDebuffsChanged();
}

void FDebuffData::RemoveDebuff(uint8 Level)
{
	if (!ensureAlwaysMsgf(Level > 0, TEXT("Cannot remove a debuff of level 0")))
	{
		return;
	}

	if (Debuffs.Contains(Level))
	{
		Debuffs.RemoveSingle(Level);
		OnDebuffsChanged();
	}
}

void FDebuffData::RemoveAllDebuffs()
{
	if (Debuffs.Num() > 0)
	{
		Debuffs.Reset();
		OnDebuffsChanged();
	}
}

void FDebuffData::RemoveAllDebuffsByLevel(uint8 Level)
{
	if (GetNumDebuffsByLevel(Level) > 0)
	{
		Debuffs.Remove(Level);
		OnDebuffsChanged();
	}
}

void FDebuffData::RemoveAllDebuffsExceptLevel(uint8 Level)
{
	Debuffs.RemoveAll([Level](uint8 DebuffLevel)
	{
		return DebuffLevel != Level;
	});
}

void FDebuffData::SetDebuffs(const TArray<uint8>& NewDebuffs)
{
	if (Debuffs != NewDebuffs)
	{
		Debuffs = NewDebuffs;
		OnDebuffsChanged();
	}
}

void FDebuffData::SetDebuffLevel(uint8 Level)
{
	if (!CharacterOwner.IsValid())
	{
		return;
	}
	
	if (Level != DebuffLevel)
	{
		const uint8 PrevLevel = DebuffLevel;
		DebuffLevel = Level;

		CharacterOwner->OnDebuffed(DebuffType, DebuffLevel, PrevLevel);
		
		// @TODO if (HonorCharacterOwner->HasAuthority())
		// {
		// 	// Replicate to sim proxies
		// 	HonorCharacterOwner->SimulatedSnareLevel = SnareLevel;
		// }
	}
}

void FDebuffData::OnDebuffsChanged()
{
	// Determine debuff level
	uint8 NewLevel = 0;
	switch (LevelMethod)
	{
	case EDebuffLevelMethod::Max:
		NewLevel = Debuffs.Num() > 0 ? *Algo::MaxElement(Debuffs) : 0;
		break;
	case EDebuffLevelMethod::Stack:
		for (const uint8& Level : Debuffs)
		{
			NewLevel += Level;
		}
		break;
	case EDebuffLevelMethod::Average:
		NewLevel = Debuffs.Num() > 0 ? (uint8)(Algo::Accumulate(Debuffs, 0) / Debuffs.Num()) : 0;
		break;
	}
	
	SetDebuffLevel(NewLevel);
}

bool FDebuffData::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	// Serialize the debuff level
	SerializeOptionalValue<uint8>(Ar.IsSaving(), Ar, DebuffLevel, 0);

	// Serialize the number of elements
	int32 NumDebuffs = Debuffs.Num();
	if (Ar.IsSaving())
	{
		NumDebuffs = FMath::Min(MaxDebuffs, NumDebuffs);
	}
	Ar << NumDebuffs;

	// Resize the array if needed
	if (Ar.IsLoading())
	{
		if (!ensureMsgf(NumDebuffs <= MaxDebuffs,
			TEXT("Deserializing debuff %s array with %d elements when max is %d -- Check packet serialization logic"), *DebuffType.ToString(), NumDebuffs, MaxDebuffs))
		{
			NumDebuffs = MaxDebuffs;
		}
		Debuffs.SetNum(NumDebuffs);
	}

	// Serialize the elements
	for (int32 i = 0; i < NumDebuffs; ++i)
	{
		Ar << Debuffs[i];
	}

	return !Ar.IsError();
}
