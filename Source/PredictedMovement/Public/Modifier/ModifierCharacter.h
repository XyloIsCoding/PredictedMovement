﻿// Copyright (c) 2023 Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ModifierCharacter.generated.h"

enum class EBoost : uint8;
enum class ESnare : uint8;
struct FGameplayTag;
class UModifierMovement;

UCLASS()
class PREDICTEDMOVEMENT_API AModifierCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Movement component used for movement logic in various movement modes (walking, falling, etc), containing relevant settings and functions to control movement. */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UModifierMovement> ModifierMovement;

	friend class FSavedMove_Character_Modifier;
protected:
	FORCEINLINE UModifierMovement* GetModifierCharacterMovement() const { return ModifierMovement; }

public:
	AModifierCharacter(const FObjectInitializer& FObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void OnModifierChanged(const FGameplayTag& ModifierType, uint8 ModifierLevel, uint8 PrevModifierLevel);
	virtual void OnModifierAdded(const FGameplayTag& ModifierType, uint8 ModifierLevel, uint8 PrevModifierLevel);
	virtual void OnModifierRemoved(const FGameplayTag& ModifierType, uint8 ModifierLevel, uint8 PrevModifierLevel);

	UFUNCTION(BlueprintImplementableEvent, Category=Character, meta=(DisplayName="On Modifier Added"))
	void K2_OnModifierAdded(const FGameplayTag& ModifierType, uint8 ModifierLevel, uint8 PrevModifierLevel);

	UFUNCTION(BlueprintImplementableEvent, Category=Character, meta=(DisplayName="On Modifier Removed"))
	void K2_OnModifierRemoved(const FGameplayTag& ModifierType, uint8 ModifierLevel, uint8 PrevModifierLevel);

protected:
	/* Boost (Non-Generic) Implementation -- Implement per-modifier type */
	
	UPROPERTY(ReplicatedUsing=OnRep_SimulatedBoost)
	uint8 SimulatedBoost;

	UFUNCTION()
	void OnRep_SimulatedBoost(uint8 PrevSimulatedBoost);

	UFUNCTION(BlueprintCallable, Category=Character)
	void Boost(FGameplayTag BoostLevel);

	UFUNCTION(BlueprintCallable, Category=Character)
	void RemoveBoost(FGameplayTag BoostLevel);

	UFUNCTION(BlueprintPure, Category=Character)
	bool IsBoosted() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	FGameplayTag GetBoostLevel() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	int32 GetNumBoosts() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	int32 GetNumBoostsByLevel(FGameplayTag BoostLevel) const;
	
	/* ~Boost (Non-Generic) Implementation */
	
protected:
	/* Snare (Non-Generic) Implementation -- Implement per-modifier type */
	
	UPROPERTY(ReplicatedUsing=OnRep_SimulatedSnare)
	uint8 SimulatedSnare;

	UFUNCTION()
	void OnRep_SimulatedSnare(uint8 PrevSimulatedSnare);

	UFUNCTION(BlueprintCallable, Category=Character)
	void Snare(FGameplayTag SnareLevel);

	UFUNCTION(BlueprintCallable, Category=Character)
	void RemoveSnare(FGameplayTag SnareLevel);

	UFUNCTION(BlueprintPure, Category=Character)
	bool IsSnared() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	FGameplayTag GetSnareLevel() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	int32 GetNumSnares() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	int32 GetNumSnaresByLevel(FGameplayTag SnareLevel) const;
	
	/* ~Snare (Non-Generic) Implementation */
};
