﻿// Copyright (c) 2023 Jared Taylor. All Rights Reserved.


#include "Stamina/StaminaMovement.h"

#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(StaminaMovement)

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

FStaminaNetworkMoveDataContainer::FStaminaNetworkMoveDataContainer()
{
    NewMoveData = &MoveData[0];
    PendingMoveData = &MoveData[1];
    OldMoveData = &MoveData[2];
}

void FStaminaNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
    Super::ClientFillNetworkMoveData(ClientMove, MoveType);
 
    Stamina = static_cast<const FSavedMove_Character_Stamina&>(ClientMove).Stamina;
}

bool FStaminaNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
    Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
    
    SerializeOptionalValue<float>(Ar.IsSaving(), Ar, Stamina, 0.f);
    return !Ar.IsError();
}

UStaminaMovement::UStaminaMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetMoveResponseDataContainer(StaminaMoveResponseDataContainer);
    SetNetworkMoveDataContainer(StaminaMoveDataContainer);

	NetworkStaminaCorrectionThreshold = 2.f;
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
	if (FMath::IsNearlyZero(Stamina))
	{
		Stamina = 0.f;
		if (!bStaminaDrained)
		{
			SetStaminaDrained(true);
		}
	}
	// This will need to change if not using MaxStamina for recovery, here is an example (commented out) that uses
	// 10% instead; to use this, comment out the existing else if statement, and change the 0.1f to the percentage
	// you want to use (0.1f is 10%)
	//
	// else if (bStaminaDrained && Stamina >= MaxStamina * 0.1f)
	// {
	// 	SetStaminaDrained(false);
	// }
	else if (FMath::IsNearlyEqual(Stamina, MaxStamina))
	{
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

	const FSavedMove_Character_Stamina* SavedOldMove = static_cast<const FSavedMove_Character_Stamina*>(OldMove);

	if (UStaminaMovement* MoveComp = C ? Cast<UStaminaMovement>(C->GetCharacterMovement()) : nullptr)
	{
		MoveComp->SetStamina(SavedOldMove->Stamina);
		MoveComp->SetStaminaDrained(SavedOldMove->bStaminaDrained);
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

void UStaminaMovement::OnClientCorrectionReceived(FNetworkPredictionData_Client_Character& ClientData, float TimeStamp,
	FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase,
	bool bBaseRelativePosition, uint8 ServerMovementMode
#if WITH_ARBITRARY_GRAVITY
	, FVector ServerGravityDirection)
#else
	)
#endif
{
	// ClientHandleMoveResponse() ➜ ClientAdjustPosition_Implementation() ➜ OnClientCorrectionReceived()
	const FStaminaMoveResponseDataContainer& StaminaMoveResponse = static_cast<const FStaminaMoveResponseDataContainer&>(GetMoveResponseDataContainer());
	
	SetStamina(StaminaMoveResponse.Stamina);
	SetStaminaDrained(StaminaMoveResponse.bStaminaDrained);

	Super::OnClientCorrectionReceived(ClientData, TimeStamp, NewLocation, NewVelocity, NewBase, NewBaseBoneName,
	bHasBase, bBaseRelativePosition, ServerMovementMode
#if WITH_ARBITRARY_GRAVITY
	, ServerGravityDirection);
#else
	);
#endif
}

bool UStaminaMovement::ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode)
{
    if (Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation, RelativeClientLocation, ClientMovementBase, ClientBaseBoneName, ClientMovementMode))
    {
        return true;
    }
    
	// This will trigger a client correction if the Stamina value in the Client differs NetworkStaminaCorrectionThreshold (2.f default) units from the one in the server
	// Desyncs can happen if we set the Stamina directly in Gameplay code (ie: GAS)
    const FStaminaNetworkMoveData* CurrentMoveData = static_cast<const FStaminaNetworkMoveData*>(GetCurrentNetworkMoveData());
    if (!FMath::IsNearlyEqual(CurrentMoveData->Stamina, Stamina, NetworkStaminaCorrectionThreshold))
    {
        return true;
    }
    
    return false;
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