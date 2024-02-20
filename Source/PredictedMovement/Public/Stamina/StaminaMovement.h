// Copyright (c) 2023 Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sprint/SprintMovement.h"
#include "StaminaMovement.generated.h"

#define WITH_ARBITRARY_GRAVITY ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3)

struct PREDICTEDMOVEMENT_API FStaminaMoveResponseDataContainer : FCharacterMoveResponseDataContainer
{
	using Super = FCharacterMoveResponseDataContainer;
	
	virtual void ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap) override;

	float Stamina;
	bool bStaminaDrained;
};

struct PREDICTEDMOVEMENT_API FStaminaNetworkMoveData : public FCharacterNetworkMoveData
{
public:
    typedef FCharacterNetworkMoveData Super;
 
    FStaminaNetworkMoveData()
        : Stamina(0)
    {

    }
 
    virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
    virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;
 
    float Stamina;
 
};
 
struct PREDICTEDMOVEMENT_API FStaminaNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
public:
    typedef FCharacterNetworkMoveDataContainer Super;
 
    FStaminaNetworkMoveDataContainer();
 
private:
    FStaminaNetworkMoveData MoveData[3];
};

/**
 * Add a void CalcStamina(float DeltaTime) function to your UCharacterMovementComponent and call it before Super after
 * overriding CalcVelocity.
 * 
 * You will want to implement what happens based on the stamina yourself, eg. override GetMaxSpeed to move slowly
 * when bStaminaDrained.
 *
 * Override OnStaminaChanged to call (or not) SetStaminaDrained based on the needs of your project. Most games will
 * want to make use of the drain state to prevent rapid sprint re-entry on tiny amounts of regenerated stamina. However,
 * unless you require the stamina to completely refill before sprinting again, then you'll want to override
 * OnStaminaChanged and change FMath::IsNearlyEqual(Stamina, MaxStamina) to multiply MaxStamina by the percentage
 * that must be regained before re-entry (eg. MaxStamina * 0.1f is 10%).
 *
 * Nothing is presumed about regenerating or draining stamina, if you want to implement those, do it in CalcVelocity or
 * at least PerformMovement - CalcVelocity stems from PerformMovement but exists within the physics subticks for greater
 * accuracy.
 *
 * If used with sprinting, OnStaminaDrained() should be overridden to call USprintMovement::UnSprint(). If you don't
 * do this, the greater accuracy of CalcVelocity is lost because it cannot stop sprinting between frames.
 *
 * You will need to handle any changes to MaxStamina, it is not predicted here.
 *
 * GAS can modify the Stamina (by calling SetStamina(), nothing special required) and it shouldn't desync, however
 * if you have any delay between the ability activating and the stamina being consumed it will likely desync; the
 * solution is to call GetCharacterMovement()->FlushServerMoves() from the Character.
 * It is worthwhile to expose this to blueprint.
 *
 * This is not designed to work with blueprint, at all, anything you want exposed to blueprint you will need to do it
 * Better yet, add accessors from your Character and perhaps a broadcast event for UI to use.
 *
 * This solution is provided by Cedric 'eXi' Neukirchen and has been repurposed for net predicted Stamina.
 */
UCLASS()
class PREDICTEDMOVEMENT_API UStaminaMovement : public USprintMovement
{
	GENERATED_BODY()

public:
	/** Maximum stamina difference that is allowed between client and server before a correction occurs. */
	UPROPERTY(Category="Character Movement (Networking)", EditDefaultsOnly, meta=(ClampMin="0.0", UIMin="0.0"))
	float NetworkStaminaCorrectionThreshold;
	
public:
	UStaminaMovement(const FObjectInitializer& ObjectInitializer);

protected:
	/** THIS SHOULD ONLY BE MODIFIED IN DERIVED CLASSES FROM OnStaminaChanged AND NOWHERE ELSE */
	UPROPERTY()
	float Stamina;

private:
	UPROPERTY()
	float MaxStamina;

	UPROPERTY()
	bool bStaminaDrained;

public:
	float GetStamina() const { return Stamina; }
	float GetMaxStamina() const { return MaxStamina; }
	bool IsStaminaDrained() const { return bStaminaDrained; }

	void SetStamina(float NewStamina);

	void SetMaxStamina(float NewMaxStamina);

	void SetStaminaDrained(bool bNewValue);

protected:
	/*
	 * Drain state entry and exit is handled here. Drain state is used to prevent rapid re-entry of sprinting or other
	 * such abilities before sufficient stamina has regenerated. However, in the default implementation, 100%
	 * stamina must be regenerated. Consider overriding this, check the implementation's comment for more information.
	 */
	virtual void OnStaminaChanged(float PrevValue, float NewValue);

	virtual void OnMaxStaminaChanged(float PrevValue, float NewValue) {}
	virtual void OnStaminaDrained() {}
	virtual void OnStaminaDrainRecovered() {}

private:
	FStaminaMoveResponseDataContainer StaminaMoveResponseDataContainer;

	FStaminaNetworkMoveDataContainer StaminaMoveDataContainer;
	
public:
	virtual void OnClientCorrectionReceived(FNetworkPredictionData_Client_Character& ClientData, float TimeStamp,
	FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase,
	bool bBaseRelativePosition, uint8 ServerMovementMode
#if WITH_ARBITRARY_GRAVITY
	, FVector ServerGravityDirection) override;
#else
	) override;
#endif
	
	virtual bool ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel,
		const FVector& ClientWorldLocation, const FVector& RelativeClientLocation,
		UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode) override;

	/** Get prediction data for a client game. Should not be used if not running as a client. Allocates the data on demand and can be overridden to allocate a custom override if desired. Result must be a FNetworkPredictionData_Client_Character. */
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
};

class PREDICTEDMOVEMENT_API FSavedMove_Character_Stamina : public FSavedMove_Character_Sprint
{
	using Super = FSavedMove_Character_Sprint;
	
public:
	FSavedMove_Character_Stamina()
		: bStaminaDrained(0)
		, Stamina(0)
	{
	}

	virtual ~FSavedMove_Character_Stamina() override
	{}

	uint32 bStaminaDrained : 1;
	float Stamina;

	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation) override;
	virtual void Clear() override;
	virtual void SetInitialPosition(ACharacter* C) override;
};

class PREDICTEDMOVEMENT_API FNetworkPredictionData_Client_Character_Stamina : public FNetworkPredictionData_Client_Character_Sprint
{
	using Super = FNetworkPredictionData_Client_Character_Sprint;

public:
	FNetworkPredictionData_Client_Character_Stamina(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
	{}

	virtual FSavedMovePtr AllocateNewMove() override;
};
