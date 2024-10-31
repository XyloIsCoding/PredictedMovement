// Copyright (c) 2023 Jared Taylor. All Rights Reserved.


#include "Debuff/DebuffCharacter.h"

#include "Debuff/DebuffMovement.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DebuffCharacter)

ADebuffCharacter::ADebuffCharacter(const FObjectInitializer& FObjectInitializer)
	: Super(FObjectInitializer.SetDefaultSubobjectClass<UDebuffMovement>(CharacterMovementComponentName))
{
	DebuffMovement = Cast<UDebuffMovement>(GetCharacterMovement());
}

void ADebuffCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ADebuffCharacter, SimulatedSnare, COND_SimulatedOnly);
}

void ADebuffCharacter::OnDebuffed(const FGameplayTag& DebuffType, uint8 DebuffLevel, uint8 PrevDebuffLevel)
{
	// Events for when a debuff is added or removed
	if (DebuffLevel > 0 && PrevDebuffLevel == 0)
	{
		OnDebuffAdded(DebuffType, DebuffLevel);
	}
	else if (DebuffLevel == 0 && PrevDebuffLevel > 0)
	{
		OnDebuffRemoved(DebuffType, PrevDebuffLevel);
	}

	// Replicate to simulated proxies
	if (HasAuthority())
	{
		 SimulatedSnare = DebuffLevel;
	}
}

void ADebuffCharacter::OnDebuffAdded(const FGameplayTag& DebuffType, uint8 DebuffLevel)
{
	// @TIP: Add Loose Gameplay Tag Here (Not Replicated)

	K2_OnDebuffAdded(DebuffType, DebuffLevel, 0);
}

void ADebuffCharacter::OnDebuffRemoved(const FGameplayTag& DebuffType, uint8 DebuffLevel)
{
	// @TIP: Remove Loose Gameplay Tag Here (Not Replicated)

	K2_OnDebuffRemoved(DebuffType, DebuffLevel, 0);
}

void ADebuffCharacter::OnRep_SimulatedSnare(uint8 PrevSimulatedSnare)
{
	if (DebuffMovement)
	{
		DebuffMovement->Snare.DebuffLevel = SimulatedSnare;
		DebuffMovement->bNetworkUpdateReceived = true;
		
		OnDebuffed(FDebuffTags::Debuff_Type_Snare, SimulatedSnare, PrevSimulatedSnare);
	}
}

void ADebuffCharacter::Snare(ESnare SnareLevel)
{
	if (DebuffMovement)
	{
		DebuffMovement->Snare.Debuff(static_cast<uint8>(SnareLevel));
	}
}

void ADebuffCharacter::RemoveSnare(ESnare SnareLevel)
{
	if (DebuffMovement)
	{
		DebuffMovement->Snare.RemoveDebuff(static_cast<uint8>(SnareLevel));
	}
}

bool ADebuffCharacter::IsSnared() const
{
	return DebuffMovement && DebuffMovement->Snare.IsDebuffed();
}

ESnare ADebuffCharacter::GetSnareLevel() const
{
	return DebuffMovement ? DebuffMovement->Snare.GetDebuffLevel<ESnare>() : ESnare::None;
}

int32 ADebuffCharacter::GetNumSnares() const
{
	return DebuffMovement ? DebuffMovement->Snare.GetNumDebuffs() : 0;
}

int32 ADebuffCharacter::GetNumSnaresByLevel(ESnare SnareLevel) const
{
	return DebuffMovement ? DebuffMovement->Snare.GetNumDebuffsByLevel(static_cast<uint8>(SnareLevel)) : 0;
}
