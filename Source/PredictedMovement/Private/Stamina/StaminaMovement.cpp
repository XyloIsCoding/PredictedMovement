﻿// Copyright (c) 2023 Jared Taylor. All Rights Reserved.


#include "Stamina/StaminaMovement.h"

#include "GameFramework/Character.h"

void FStaminaMoveResponseDataContainer::ServerFillResponseData(
	const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment)
{
	Super::ServerFillResponseData(CharacterMovement, PendingAdjustment);

	const UStaminaMovement* MoveComp = Cast<UStaminaMovement>(&CharacterMovement);

	bStaminaDrained = MoveComp->IsStaminaDrained();
	Stamina = MoveComp->GetStamina();
}

bool FStaminaMoveResponseDataContainer::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar,
	UPackageMap* PackageMap)
{
	if (!Super::Serialize(CharacterMovement, Ar, PackageMap))
	{
		return false;
	}

	if (IsCorrection())
	{
		Ar << Stamina;
		Ar << bStaminaDrained;
	}

	return !Ar.IsError();
}

UStaminaMovement::UStaminaMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetMoveResponseDataContainer(StaminaMoveResponseDataContainer);
}

void UStaminaMovement::SetStamina(float NewStamina)
{
	const float PrevStamina = Stamina;
	Stamina = FMath::Clamp(NewStamina, 0.f, MaxStamina);
	if (CharacterOwner != nullptr)
	{
		if (!FMath::IsNearlyEqual(PrevStamina, Stamina))
		{
			OnStaminaChanged(PrevStamina, Stamina);
		}
	}
}

void UStaminaMovement::SetMaxStamina(float NewMaxStamina)
{
	const float PrevMaxStamina = MaxStamina;
	MaxStamina = FMath::Max(0.f, NewMaxStamina);
	if (CharacterOwner != nullptr)
	{
		if (!FMath::IsNearlyEqual(PrevMaxStamina, MaxStamina))
		{
			OnMaxStaminaChanged(PrevMaxStamina, MaxStamina);
		}
	}
}

void UStaminaMovement::SetStaminaDrained(bool bNewValue)
{
	const bool bWasStaminaDrained = bStaminaDrained;
	bStaminaDrained = bNewValue;
	if (CharacterOwner != nullptr)
	{
		if (bWasStaminaDrained != bStaminaDrained)
		{
			if (bStaminaDrained)
			{
				OnStaminaDrained();
			}
			else
			{
				OnStaminaDrainRecovered();
			}
		}
	}
}

void UStaminaMovement::OnStaminaChanged(float PrevValue, float NewValue)
{
	/*
	 * Drain state entry and exit is handled here. Drain state is used to prevent rapid re-entry of sprinting or other
	 * such abilities before sufficient stamina has regenerated. However, in the default implementation, 100%
	 * stamina must be regenerated. Consider overriding this and changing FMath::IsNearlyEqual(Stamina, MaxStamina)
	 * to FMath::IsNearlyEqual(Stamina, MaxStamina * 0.1f) to require 10% regeneration (or change the 0.1f to your
	 * desired value) in the else-if scope in the function body.
	 */
	if (FMath::IsNearlyZero(Stamina))
	{
		Stamina = 0.f;
		if (!bStaminaDrained)
		{
			SetStaminaDrained(true);
		}
	}
	// eg. FMath::IsNearlyEqual(Stamina, MaxStamina * 0.1f) to require 10% regeneration
	else if (FMath::IsNearlyEqual(Stamina, MaxStamina))
	{
		// If you want to add a percentage, whether its 10% or otherwise, you will need to multiply MaxStamina here
		// eg. Stamina = MaxStamina * 0.1f;
		Stamina = MaxStamina;
		if (bStaminaDrained)
		{
			SetStaminaDrained(false);
		}
	}
}

bool FSavedMove_Character_Stamina::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter,
	float MaxDelta) const
{
	const TSharedPtr<FSavedMove_Character_Stamina>& SavedMove = StaticCastSharedPtr<FSavedMove_Character_Stamina>(NewMove);

	if (bStaminaDrained != SavedMove->bStaminaDrained)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Character_Stamina::CombineWith(const FSavedMove_Character* OldMove, ACharacter* C,
	APlayerController* PC, const FVector& OldStartLocation)
{
	Super::CombineWith(OldMove, C, PC, OldStartLocation);

	const FSavedMove_Character_Stamina* IsekaiOldMove = static_cast<const FSavedMove_Character_Stamina*>(OldMove);

	if (UStaminaMovement* MoveComp = C ? Cast<UStaminaMovement>(C->GetCharacterMovement()) : nullptr)
	{
		MoveComp->SetStamina(IsekaiOldMove->Stamina);
		MoveComp->SetStaminaDrained(IsekaiOldMove->bStaminaDrained);
	}
}

void FSavedMove_Character_Stamina::Clear()
{
	Super::Clear();

	bStaminaDrained = false;
	Stamina = 0.f;
}

void FSavedMove_Character_Stamina::SetInitialPosition(ACharacter* C)
{
	Super::SetInitialPosition(C);

	if (const UStaminaMovement* MoveComp = C ? Cast<UStaminaMovement>(C->GetCharacterMovement()) : nullptr)
	{
		bStaminaDrained = MoveComp->IsStaminaDrained();
		Stamina = MoveComp->GetStamina();
	}
}

void UStaminaMovement::ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse)
{
	if (MoveResponse.IsCorrection())
	{
		if (!MoveResponse.bRootMotionSourceCorrection && !MoveResponse.bRootMotionMontageCorrection)
		{
			bool ValidMoveResponse = true;
			if (!HasValidData() || !IsActive())
			{
				ValidMoveResponse = false;
			}

			const FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
			check(ClientData);

			const FStaminaMoveResponseDataContainer& StaminaMoveResponse = static_cast<const FStaminaMoveResponseDataContainer&>(MoveResponse);

			// Make sure the base actor exists on this client.
			const bool bUnresolvedBase = StaminaMoveResponse.bHasBase && (StaminaMoveResponse.ClientAdjustment.NewBase == nullptr);
			if (bUnresolvedBase)
			{
				if (StaminaMoveResponse.ClientAdjustment.bBaseRelativePosition)
				{
					ValidMoveResponse = false;
				}
			}

			// Make sure the SavedMove still exists
			const int32 MoveIndex = ClientData->GetSavedMoveIndex(StaminaMoveResponse.ClientAdjustment.TimeStamp);
			if (MoveIndex == INDEX_NONE)
			{
				ValidMoveResponse = false;
			}

			// This is a bit weird, but the actual function the adjust the client position doesn't get the MoveResponse, but single variables
			// So we can't really override that one as we need the full container.
			// The Adjust Client Function does however filter actually correcting things based on the two conditions above, so we have to mimic them
			if (ValidMoveResponse)
			{
#if !UE_BUILD_SHIPPING
				const IConsoleVariable* VarNetShowCorrections = IConsoleManager::Get().FindConsoleVariable(*FString("p.NetShowCorrections"));
				if (VarNetShowCorrections != nullptr && VarNetShowCorrections->GetInt() != 0)
				{
					UE_LOG(LogTemp, Error, TEXT("[%s] Client Received Correction."), *FString(__FUNCTION__));
				}
#endif // !UE_BUILD_SHIPPPING

				SetStamina(StaminaMoveResponse.Stamina);
				SetStaminaDrained(StaminaMoveResponse.bStaminaDrained);
			}
		}
	}
	
	Super::ClientHandleMoveResponse(MoveResponse);
}

FNetworkPredictionData_Client* UStaminaMovement::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UStaminaMovement* MutableThis = const_cast<UStaminaMovement*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Character_Stamina(*this);
	}

	return ClientPredictionData;
}

FSavedMovePtr FNetworkPredictionData_Client_Character_Stamina::AllocateNewMove()
{
	return MakeShared<FSavedMove_Character_Stamina>();
}