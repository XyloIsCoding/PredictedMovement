// Copyright (c) 2023 Jared Taylor. All Rights Reserved.


#include "Debuff/DebuffMovement.h"

#include "Debuff/DebuffCharacter.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DebuffMovement)

void FDebuffMoveResponseDataContainer::ServerFillResponseData(
	const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment)
{
	Super::ServerFillResponseData(CharacterMovement, PendingAdjustment);

	// Server ➜ Client
	const UDebuffMovement* MoveComp = Cast<UDebuffMovement>(&CharacterMovement);
	Snare = MoveComp->Snare;
}

bool FDebuffMoveResponseDataContainer::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar,
	UPackageMap* PackageMap)
{
	if (!Super::Serialize(CharacterMovement, Ar, PackageMap))
	{
		return false;
	}

	// Server ➜ Client
	if (IsCorrection())
	{
		bool bOutSuccess;
		Snare.NetSerialize(Ar, PackageMap, bOutSuccess);
	}

	return !Ar.IsError();
}

FDebuffNetworkMoveDataContainer::FDebuffNetworkMoveDataContainer()
{
    NewMoveData = &MoveData[0];
    PendingMoveData = &MoveData[1];
    OldMoveData = &MoveData[2];
}

void FDebuffNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
    Super::ClientFillNetworkMoveData(ClientMove, MoveType);

	// Client ➜ Server
	const FSavedMove_Character_Debuff& SavedMove = static_cast<const FSavedMove_Character_Debuff&>(ClientMove);
	Snare = SavedMove.Snare;
}

bool FDebuffNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
    Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);

	// Client ➜ Server
	bool bOutSuccess;
	Snare.NetSerialize(Ar, PackageMap, bOutSuccess);
	
    return !Ar.IsError();
}

UDebuffMovement::UDebuffMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetMoveResponseDataContainer(DebuffMoveResponseDataContainer);
    SetNetworkMoveDataContainer(DebuffMoveDataContainer);
}

void UDebuffMovement::PostLoad()
{
	Super::PostLoad();

	ADebuffCharacter* DebuffCharacter = Cast<ADebuffCharacter>(PawnOwner);
	Snare.Initialize(DebuffCharacter);
}

void UDebuffMovement::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	Super::SetUpdatedComponent(NewUpdatedComponent);

	ADebuffCharacter* DebuffCharacter = Cast<ADebuffCharacter>(PawnOwner);
	Snare.Initialize(DebuffCharacter);
}

bool FSavedMove_Character_Debuff::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter,
	float MaxDelta) const
{
	const TSharedPtr<FSavedMove_Character_Debuff>& SavedMove = StaticCastSharedPtr<FSavedMove_Character_Debuff>(NewMove);

	if (Snare != SavedMove->Snare)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Character_Debuff::CombineWith(const FSavedMove_Character* OldMove, ACharacter* C,
	APlayerController* PC, const FVector& OldStartLocation)
{
	Super::CombineWith(OldMove, C, PC, OldStartLocation);

	const FSavedMove_Character_Debuff* SavedOldMove = static_cast<const FSavedMove_Character_Debuff*>(OldMove);

	if (UDebuffMovement* MoveComp = C ? Cast<UDebuffMovement>(C->GetCharacterMovement()) : nullptr)
	{
		MoveComp->Snare = SavedOldMove->Snare;
	}
}

void FSavedMove_Character_Debuff::Clear()
{
	Super::Clear();
	Snare = {};
}

void FSavedMove_Character_Debuff::SetInitialPosition(ACharacter* C)
{
	Super::SetInitialPosition(C);

	if (const UDebuffMovement* MoveComp = C ? Cast<UDebuffMovement>(C->GetCharacterMovement()) : nullptr)
	{
		Snare = MoveComp->Snare;
	}
}

float UDebuffMovement::GetMaxSpeedScalar() const
{
	switch (Snare.GetDebuffLevel<ESnare>())
	{
	case ESnare::Snare_25: return SnaredScalar_25;
	case ESnare::Snare_50: return SnaredScalar_50;
	case ESnare::Snare_75: return SnaredScalar_75;
	default: return 1.f;
	}
}

float UDebuffMovement::GetRootMotionTranslationScalar() const
{
	return bSnareAffectsRootMotion ? GetMaxSpeedScalar() : 1.f;
}

float UDebuffMovement::GetMaxSpeed() const
{
	return Super::GetMaxSpeed() * GetMaxSpeedScalar();
}

void UDebuffMovement::TickCharacterPose(float DeltaTime)
{
	/*
	 * Epic made ACharacter::GetAnimRootMotionTranslationScale() non-virtual which was silly,
	 * so we have to duplicate the entire function. All we do here is scale
	 * CharacterOwner->GetAnimRootMotionTranslationScale() by GetRootMotionTranslationScalar()
	 * 
	 * This means our snare affects root motion as well.
	 */
	
	if (DeltaTime < UCharacterMovementComponent::MIN_TICK_TIME)
	{
		return;
	}

	check(CharacterOwner && CharacterOwner->GetMesh());
	USkeletalMeshComponent* CharacterMesh = CharacterOwner->GetMesh();

	// bAutonomousTickPose is set, we control TickPose from the Character's Movement and Networking updates, and bypass the Component's update.
	// (Or Simulating Root Motion for remote clients)
	CharacterMesh->bIsAutonomousTickPose = true;

	if (CharacterMesh->ShouldTickPose())
	{
		// Keep track of if we're playing root motion, just in case the root motion montage ends this frame.
		const bool bWasPlayingRootMotion = CharacterOwner->IsPlayingRootMotion();

		CharacterMesh->TickPose(DeltaTime, true);

		// Grab root motion now that we have ticked the pose
		if (CharacterOwner->IsPlayingRootMotion() || bWasPlayingRootMotion)
		{
			FRootMotionMovementParams RootMotion = CharacterMesh->ConsumeRootMotion();
			if (RootMotion.bHasRootMotion)
			{
				RootMotion.ScaleRootMotionTranslation(CharacterOwner->GetAnimRootMotionTranslationScale() * GetRootMotionTranslationScalar());
				RootMotionParams.Accumulate(RootMotion);
			}

#if !(UE_BUILD_SHIPPING)
			// Debugging
			{
				FAnimMontageInstance* RootMotionMontageInstance = CharacterOwner->GetRootMotionAnimMontageInstance();
				UE_LOG(LogRootMotion, Log, TEXT("UCharacterMovementComponent::TickCharacterPose Role: %s, RootMotionMontage: %s, MontagePos: %f, DeltaTime: %f, ExtractedRootMotion: %s, AccumulatedRootMotion: %s")
					, *UEnum::GetValueAsString(TEXT("Engine.ENetRole"), CharacterOwner->GetLocalRole())
					, *GetNameSafe(RootMotionMontageInstance ? RootMotionMontageInstance->Montage : NULL)
					, RootMotionMontageInstance ? RootMotionMontageInstance->GetPosition() : -1.f
					, DeltaTime
					, *RootMotion.GetRootMotionTransform().GetTranslation().ToCompactString()
					, *RootMotionParams.GetRootMotionTransform().GetTranslation().ToCompactString()
					);
			}
#endif // !(UE_BUILD_SHIPPING)
		}
	}

	CharacterMesh->bIsAutonomousTickPose = false;
}

void UDebuffMovement::OnClientCorrectionReceived(FNetworkPredictionData_Client_Character& ClientData, float TimeStamp,
	FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase,
	bool bBaseRelativePosition, uint8 ServerMovementMode
#if UE_5_03_OR_LATER
	, FVector ServerGravityDirection)
#else
	)
#endif
{
	// ClientHandleMoveResponse() ➜ ClientAdjustPosition_Implementation() ➜ OnClientCorrectionReceived()
	const FDebuffMoveResponseDataContainer& DebuffMoveResponse = static_cast<const FDebuffMoveResponseDataContainer&>(GetMoveResponseDataContainer());

	if (Snare != DebuffMoveResponse.Snare)
	{
		// Snare has changed, apply correction
		Snare = DebuffMoveResponse.Snare;
		Snare.OnDebuffsChanged();
	}

	Super::OnClientCorrectionReceived(ClientData, TimeStamp, NewLocation, NewVelocity, NewBase, NewBaseBoneName,
	bHasBase, bBaseRelativePosition, ServerMovementMode
#if UE_5_03_OR_LATER
	, ServerGravityDirection);
#else
	);
#endif
}

bool UDebuffMovement::ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode)
{
    if (Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation, RelativeClientLocation, ClientMovementBase, ClientBaseBoneName, ClientMovementMode))
    {
        return true;
    }

	// Trigger client correction if debuffs have different DebuffLevel or Debuffs
	// De-syncs can happen when we set the Debuff directly in Gameplay code (i.e. GAS)
	
    const FDebuffNetworkMoveData* CurrentMoveData = static_cast<const FDebuffNetworkMoveData*>(GetCurrentNetworkMoveData());
    if (CurrentMoveData->Snare != Snare)
	{
		return true;
	}
    
    return false;
}

FNetworkPredictionData_Client* UDebuffMovement::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UDebuffMovement* MutableThis = const_cast<UDebuffMovement*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Character_Debuff(*this);
	}

	return ClientPredictionData;
}

FSavedMovePtr FNetworkPredictionData_Client_Character_Debuff::AllocateNewMove()
{
	return MakeShared<FSavedMove_Character_Debuff>();
}