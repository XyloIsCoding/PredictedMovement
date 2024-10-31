// Copyright (c) 2023 Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DebuffCharacter.generated.h"

enum class ESnare : uint8;
struct FGameplayTag;
class UDebuffMovement;

UCLASS()
class PREDICTEDMOVEMENT_API ADebuffCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Movement component used for movement logic in various movement modes (walking, falling, etc), containing relevant settings and functions to control movement. */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UDebuffMovement> DebuffMovement;
	
protected:
	FORCEINLINE UDebuffMovement* GetDebuffCharacterMovement() const { return DebuffMovement; }

public:
	ADebuffCharacter(const FObjectInitializer& FObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void OnDebuffed(const FGameplayTag& DebuffType, uint8 DebuffLevel, uint8 PrevDebuffLevel);
	virtual void OnDebuffAdded(const FGameplayTag& DebuffType, uint8 DebuffLevel);
	virtual void OnDebuffRemoved(const FGameplayTag& DebuffType, uint8 DebuffLevel);

	UFUNCTION(BlueprintImplementableEvent, Category=Character, meta=(DisplayName="On Debuff Added"))
	void K2_OnDebuffAdded(const FGameplayTag& DebuffType, uint8 DebuffLevel, uint8 PrevDebuffLevel);

	UFUNCTION(BlueprintImplementableEvent, Category=Character, meta=(DisplayName="On Debuff Removed"))
	void K2_OnDebuffRemoved(const FGameplayTag& DebuffType, uint8 DebuffLevel, uint8 PrevDebuffLevel);

protected:
	/* Snare (Non-Generic) Implementation -- Implement per-debuff type */
	
	UPROPERTY(ReplicatedUsing=OnRep_SimulatedSnare)
	uint8 SimulatedSnare;

	UFUNCTION()
	void OnRep_SimulatedSnare(uint8 PrevSimulatedSnare);

	UFUNCTION(BlueprintCallable, Category=Character)
	void Snare(ESnare SnareLevel);

	UFUNCTION(BlueprintCallable, Category=Character)
	void RemoveSnare(ESnare SnareLevel);

	UFUNCTION(BlueprintPure, Category=Character)
	bool IsSnared() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	ESnare GetSnareLevel() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	int32 GetNumSnares() const;

	UFUNCTION(BlueprintCallable, Category=Character)
	int32 GetNumSnaresByLevel(ESnare SnareLevel) const;
	
	/* ~Snare (Non-Generic) Implementation */
};
