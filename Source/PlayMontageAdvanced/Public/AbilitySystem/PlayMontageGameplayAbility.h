// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "PlayMontageGameplayAbility.generated.h"

// Most of this is from GASShooter and therefore also Copyright 2024 Dan Kestranek.
// https://github.com/tranek/GASShooter

USTRUCT()
struct PLAYMONTAGEADVANCED_API FAbilityMeshMontage
{
	GENERATED_BODY()

	UPROPERTY()
	USkeletalMeshComponent* Mesh;

	UPROPERTY()
	UAnimMontage* Montage;

	FAbilityMeshMontage(USkeletalMeshComponent* InMesh = nullptr, UAnimMontage* InMontage = nullptr) 
		: Mesh(InMesh)
		, Montage(InMontage)
	{
	}
};

/**
 * 
 */
UCLASS()
class PLAYMONTAGEADVANCED_API UPlayMontageGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	// ----------------------------------------------------------------------------------------------------------------
	//	Animation Support for multiple USkeletalMeshComponents on the AvatarActor
	// ----------------------------------------------------------------------------------------------------------------

	/** Active montages being played by this ability */
	UPROPERTY()
	TArray<FAbilityMeshMontage> CurrentAbilityMeshMontages;

	bool FindAbilityMeshMontage(USkeletalMeshComponent* InMesh, FAbilityMeshMontage& InAbilityMontage);
	
	/** Returns the currently playing montage for this ability, if any */
	UFUNCTION(BlueprintCallable, Category = Animation)
	UAnimMontage* GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh);

	/** Call to set/get the current montage from a montage task. Set to allow hooking up montage events to ability events */
	virtual void SetCurrentMontageForMesh(USkeletalMeshComponent* InMesh, class UAnimMontage* InCurrentMontage);

};
