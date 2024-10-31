// Copyright (c) 2023 Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "System/PredictedMovementVersioning.h"
#include "DebuffTypes.h"
#include "DebuffMovement.generated.h"

// @TODO example of a debuff that can stack
UENUM(BlueprintType)
enum class ESnare : uint8
{
	None,
	Snare_25		UMETA(DisplayName = "Snare 25% (Reduction)", Tooltip = "25% Reduction in Movement Speed"),
	Snare_50		UMETA(DisplayName = "Snare 50% (Reduction)", Tooltip = "50% Reduction in Movement Speed"),
	Snare_75		UMETA(DisplayName = "Snare 75% (Reduction)", Tooltip = "75% Reduction in Movement Speed"),
};

struct PREDICTEDMOVEMENT_API FDebuffMoveResponseDataContainer : FCharacterMoveResponseDataContainer
{  // Server ➜ Client
	using Super = FCharacterMoveResponseDataContainer;

	FDebuffData Snare;
	
	virtual void ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap) override;
};

struct PREDICTEDMOVEMENT_API FDebuffNetworkMoveData : public FCharacterNetworkMoveData
{  // Client ➜ Server
public:
    typedef FCharacterNetworkMoveData Super;
 
    FDebuffNetworkMoveData()
    {
    	
    }

	FDebuffData Snare;
 
    virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
    virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;
};
 
struct PREDICTEDMOVEMENT_API FDebuffNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{  // Client ➜ Server
public:
    typedef FCharacterNetworkMoveDataContainer Super;
 
    FDebuffNetworkMoveDataContainer();
 
private:
    FDebuffNetworkMoveData MoveData[3];
};

/**
 * @TODO
 * This is using snares as example, but you can use it for any buff or debuff that has stacking application
 * Snare is used as an example of a debuff, you can duplicate for your own debuffs and buffs
 */
UCLASS()
class PREDICTEDMOVEMENT_API UDebuffMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UDebuffMovement(const FObjectInitializer& ObjectInitializer);

	virtual void PostLoad() override;
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;

public:
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ForceUnits="x"))
	float SnaredScalar_25 = 0.75f;

	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ForceUnits="x"))
	float SnaredScalar_50 = 0.5f;

	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ForceUnits="x"))
	float SnaredScalar_75 = 0.25f;

	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ForceUnits="x"))
	bool bSnareAffectsRootMotion = true;
	
public:
	/** Example implementation of a debuff that can stack */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite)
	FDebuffData Snare;

	UFUNCTION(BlueprintCallable, Category="Character Movement: Walking")
	ESnare GetSnareLevel() const
	{
		return Snare.GetDebuffLevel<ESnare>();
	}

	UFUNCTION(BlueprintPure, Category="Character Movement: Walking")
	bool IsSnared() const
	{
		return Snare.IsDebuffed();
	}

	virtual float GetMaxSpeedScalar() const;
	virtual float GetRootMotionTranslationScalar() const;
	virtual float GetMaxSpeed() const override;
	virtual void TickCharacterPose(float DeltaTime) override;
	
private:
	FDebuffMoveResponseDataContainer DebuffMoveResponseDataContainer;

	FDebuffNetworkMoveDataContainer DebuffMoveDataContainer;
	
public:
	virtual void OnClientCorrectionReceived(FNetworkPredictionData_Client_Character& ClientData, float TimeStamp,
	FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase,
	bool bBaseRelativePosition, uint8 ServerMovementMode
#if UE_5_03_OR_LATER
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

class PREDICTEDMOVEMENT_API FSavedMove_Character_Debuff : public FSavedMove_Character
{
	using Super = FSavedMove_Character;
	
public:
	FSavedMove_Character_Debuff()
	{
	}

	virtual ~FSavedMove_Character_Debuff() override
	{}

	FDebuffData Snare;

	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation) override;
	virtual void Clear() override;
	virtual void SetInitialPosition(ACharacter* C) override;
};

class PREDICTEDMOVEMENT_API FNetworkPredictionData_Client_Character_Debuff : public FNetworkPredictionData_Client_Character
{
	using Super = FNetworkPredictionData_Client_Character;

public:
	FNetworkPredictionData_Client_Character_Debuff(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
	{}

	virtual FSavedMovePtr AllocateNewMove() override;
};
