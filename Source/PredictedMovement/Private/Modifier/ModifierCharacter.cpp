﻿// Copyright (c) 2023 Jared Taylor. All Rights Reserved.

#include "Modifier/ModifierCharacter.h"

#include "Modifier/ModifierMovement.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModifierCharacter)

AModifierCharacter::AModifierCharacter(const FObjectInitializer& FObjectInitializer)
	: Super(FObjectInitializer.SetDefaultSubobjectClass<UModifierMovement>(CharacterMovementComponentName))
{
	ModifierMovement = Cast<UModifierMovement>(GetCharacterMovement());
}

void AModifierCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, SimulatedBoost, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(ThisClass, SimulatedSlowFall, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(ThisClass, SimulatedSnare, COND_SimulatedOnly);
}

void AModifierCharacter::OnModifierChanged(const FGameplayTag& ModifierType, uint8 ModifierLevel, uint8 PrevModifierLevel)
{
	// Events for when a modifier is added, removed or changed
	if (ModifierLevel > 0 && PrevModifierLevel == 0)
	{
		OnModifierAdded(ModifierType, ModifierLevel, PrevModifierLevel);
	}
	else if (ModifierLevel == 0 && PrevModifierLevel > 0)
	{
		OnModifierRemoved(ModifierType, ModifierLevel, PrevModifierLevel);
	}

	K2_OnModifierChanged(ModifierType, ModifierLevel, PrevModifierLevel);

	// Replicate to simulated proxies
	if (HasAuthority())
	{
		if (ModifierType == FModifierTags::Modifier_Type_Buff_Boost)
		{
			SimulatedBoost = ModifierLevel;
		}
		else if (ModifierType == FModifierTags::Modifier_Type_Buff_SlowFall)
		{
			SimulatedSlowFall = ModifierLevel;
		}
		else if (ModifierType == FModifierTags::Modifier_Type_Debuff_Snare)
		{
			SimulatedSnare = ModifierLevel;
		}
	}
}

void AModifierCharacter::OnModifierAdded(const FGameplayTag& ModifierType, uint8 ModifierLevel, uint8 PrevModifierLevel)
{
	// @TIP: Add Loose Gameplay Tag Here (Not Replicated)

	if (!ModifierMovement)
	{
		return;
	}

	if (ModifierType == FModifierTags::Modifier_Type_Buff_Boost)
	{
		ModifierMovement->OnStartBoost();
	}
	else if (ModifierType == FModifierTags::Modifier_Type_Buff_SlowFall)
	{
		ModifierMovement->OnStartSlowFall();
	}
	else if (ModifierType == FModifierTags::Modifier_Type_Debuff_Snare)
	{
		ModifierMovement->OnStartSnare();
	}

	K2_OnModifierAdded(ModifierType, ModifierLevel, PrevModifierLevel);
}

void AModifierCharacter::OnModifierRemoved(const FGameplayTag& ModifierType, uint8 ModifierLevel, uint8 PrevModifierLevel)
{
	// @TIP: Remove Loose Gameplay Tag Here (Not Replicated)

	if (ModifierType == FModifierTags::Modifier_Type_Buff_Boost)
	{
		ModifierMovement->OnEndBoost();
	}
	else if (ModifierType == FModifierTags::Modifier_Type_Buff_SlowFall)
	{
		ModifierMovement->OnEndSlowFall();
	}
	else if (ModifierType == FModifierTags::Modifier_Type_Debuff_Snare)
	{
		ModifierMovement->OnEndSnare();
	}

	K2_OnModifierRemoved(ModifierType, ModifierLevel, PrevModifierLevel);
}

/* Boost (Non-Generic) Implementation */

void AModifierCharacter::OnRep_SimulatedBoost(uint8 PrevSimulatedBoost)
{
	if (ModifierMovement)
	{
		if (SimulatedBoost > 0)
		{
			ModifierMovement->Boost.RequestedModifierLevel = SimulatedBoost;
			ModifierMovement->Boost.StartModifier(SimulatedBoost, true, true);
		}
		else
		{
			ModifierMovement->Boost.RemoveAllModifiers();
			ModifierMovement->Boost.EndModifier(true);
		}
		ModifierMovement->bNetworkUpdateReceived = true;
		
		OnModifierChanged(FModifierTags::Modifier_Type_Buff_Boost, SimulatedBoost, PrevSimulatedBoost);
	}
}

void AModifierCharacter::Boost(FGameplayTag ModifierLevel)
{
	if (ModifierMovement)
	{
		if (const uint8 Level = ModifierMovement->Boost.GetModifierLevelByte(ModifierLevel); Level != LEVEL_NONE)
		{
			ModifierMovement->Boost.AddModifier(Level);
		}
	}
}

void AModifierCharacter::RemoveBoost(FGameplayTag ModifierLevel)
{
	if (ModifierMovement)
	{
		if (const uint8 Level = ModifierMovement->Boost.GetModifierLevelByte(ModifierLevel); Level != LEVEL_NONE)
		{
			ModifierMovement->Boost.RemoveModifier(Level);
		}
	}
}

void AModifierCharacter::RemoveAllBoosts()
{
	if (ModifierMovement)
	{
		ModifierMovement->Boost.RemoveAllModifiers();
	}
}

bool AModifierCharacter::IsBoosted() const
{
	return ModifierMovement && ModifierMovement->Boost.HasModifier();
}

bool AModifierCharacter::WantsBoost() const
{
	return ModifierMovement && ModifierMovement->Boost.WantsModifier();
}

FGameplayTag AModifierCharacter::GetBoostLevel() const
{
	return ModifierMovement ? ModifierMovement->Boost.GetModifierLevel() : FGameplayTag::EmptyTag;
}

int32 AModifierCharacter::GetNumBoosts() const
{
	return ModifierMovement ? ModifierMovement->Boost.GetNumModifiers() : 0;
}

int32 AModifierCharacter::GetNumBoostsByLevel(FGameplayTag ModifierLevel) const
{
	if (ModifierMovement)
	{
		const uint8 Level = ModifierMovement->Boost.GetModifierLevelByte(ModifierLevel);
		return Level != LEVEL_NONE ? ModifierMovement->Boost.GetNumModifiersByLevel(Level) : 0;
	}
	return 0;
}

/* ~Boost (Non-Generic) Implementation */

/* Slow Fall (Non-Generic) Implementation */

void AModifierCharacter::OnRep_SimulatedSlowFall(uint8 PrevSimulatedSlowFall)
{
	if (ModifierMovement)
	{
		if (SimulatedSlowFall > 0)
		{
			ModifierMovement->SlowFall.RequestedModifierLevel = SimulatedSlowFall;
			ModifierMovement->SlowFall.StartModifier(SimulatedSlowFall, true, true);
		}
		else
		{
			ModifierMovement->SlowFall.RemoveAllModifiers();
			ModifierMovement->SlowFall.EndModifier(true);
		}
		ModifierMovement->bNetworkUpdateReceived = true;
		
		OnModifierChanged(FModifierTags::Modifier_Type_Buff_SlowFall, SimulatedSlowFall, PrevSimulatedSlowFall);
	}
}

void AModifierCharacter::SlowFall(FGameplayTag ModifierLevel)
{
	if (ModifierMovement)
	{
		if (const uint8 Level = ModifierMovement->SlowFall.GetModifierLevelByte(ModifierLevel); Level != LEVEL_NONE)
		{
			ModifierMovement->SlowFall.AddModifier(Level);
		}
	}
}

void AModifierCharacter::RemoveSlowFall(FGameplayTag ModifierLevel)
{
	if (ModifierMovement)
	{
		if (const uint8 Level = ModifierMovement->SlowFall.GetModifierLevelByte(ModifierLevel); Level != LEVEL_NONE)
		{
			ModifierMovement->SlowFall.RemoveModifier(Level);
		}
	}
}

void AModifierCharacter::RemoveAllSlowFall()
{
	if (ModifierMovement)
	{
		ModifierMovement->SlowFall.RemoveAllModifiers();
	}
}

bool AModifierCharacter::IsSlowFall() const
{
	return ModifierMovement && ModifierMovement->SlowFall.HasModifier();
}

bool AModifierCharacter::WantsSlowFall() const
{
	return ModifierMovement && ModifierMovement->SlowFall.WantsModifier();
}

FGameplayTag AModifierCharacter::GetSlowFallLevel() const
{
	return ModifierMovement ? ModifierMovement->SlowFall.GetModifierLevel() : FGameplayTag::EmptyTag;
}

int32 AModifierCharacter::GetNumSlowFalls() const
{
	return ModifierMovement ? ModifierMovement->SlowFall.GetNumModifiers() : 0;
}

int32 AModifierCharacter::GetNumSlowFallsByLevel(FGameplayTag ModifierLevel) const
{
	if (ModifierMovement)
	{
		const uint8 Level = ModifierMovement->SlowFall.GetModifierLevelByte(ModifierLevel);
		return Level != LEVEL_NONE ? ModifierMovement->SlowFall.GetNumModifiersByLevel(Level) : 0;
	}
	return 0;
}

/* ~Slow Fall (Non-Generic) Implementation */

/* Snare (Non-Generic) Implementation */

void AModifierCharacter::OnRep_SimulatedSnare(uint8 PrevSimulatedSnare)
{
	if (ModifierMovement)
	{
		if (SimulatedSnare > 0)
		{
			ModifierMovement->Snare.RequestedModifierLevel = SimulatedSnare;
			ModifierMovement->Snare.StartModifier(SimulatedSnare, true, true);
		}
		else
		{
			ModifierMovement->Snare.RemoveAllModifiers();
			ModifierMovement->Snare.EndModifier(true);
		}
		ModifierMovement->bNetworkUpdateReceived = true;
		
		OnModifierChanged(FModifierTags::Modifier_Type_Debuff_Snare, SimulatedSnare, PrevSimulatedSnare);
	}
}

void AModifierCharacter::Snare(FGameplayTag ModifierLevel)
{
	if (ModifierMovement)
	{
		if (const uint8 Level = ModifierMovement->Snare.GetModifierLevelByte(ModifierLevel); Level != LEVEL_NONE)
		{
			ModifierMovement->Snare.AddModifier(Level);
		}
	}
}

void AModifierCharacter::RemoveSnare(FGameplayTag ModifierLevel)
{
	if (ModifierMovement)
	{
		if (const uint8 Level = ModifierMovement->Snare.GetModifierLevelByte(ModifierLevel); Level != LEVEL_NONE)
		{
			ModifierMovement->Snare.RemoveModifier(Level);
		}
	}
}

void AModifierCharacter::RemoveAllSnares()
{
	if (ModifierMovement)
	{
		ModifierMovement->Snare.RemoveAllModifiers();
	}
}

bool AModifierCharacter::IsSnared() const
{
	return ModifierMovement && ModifierMovement->Snare.HasModifier();
}

bool AModifierCharacter::WantsSnare() const
{
	return ModifierMovement && ModifierMovement->Snare.WantsModifier();
}

FGameplayTag AModifierCharacter::GetSnareLevel() const
{
	return ModifierMovement ? ModifierMovement->Snare.GetModifierLevel() : FGameplayTag::EmptyTag;
}

int32 AModifierCharacter::GetNumSnares() const
{
	return ModifierMovement ? ModifierMovement->Snare.GetNumModifiers() : 0;
}

int32 AModifierCharacter::GetNumSnaresByLevel(FGameplayTag ModifierLevel) const
{
	if (ModifierMovement)
	{
		const uint8 Level = ModifierMovement->Snare.GetModifierLevelByte(ModifierLevel);
		return Level != LEVEL_NONE ? ModifierMovement->Snare.GetNumModifiersByLevel(Level) : 0;
	}
	return 0;
}

/* ~Snare (Non-Generic) Implementation */