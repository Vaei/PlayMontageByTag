// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlayMontageByTagLib.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;
struct FDrivenMontages;
struct FDrivenMontagePair;
/**
 * 
 */
UCLASS()
class PLAYMONTAGEBYTAG_API UPlayMontageByTagLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** @return PlayRate to conform Montage to Duration */
	static float GetMontagePlayRateScaledByDuration(const UAnimMontage* Montage, float Duration);

	// Play
	
	static bool ShouldPlayLocalDrivenMontages(const AActor* AvatarActor);

	static void PlayDrivenMontage(float Duration, float Rate, const FName& StartSection, const FDrivenMontagePair& Montage);

	static void PlayDrivenMontages(const AActor* AvatarActor, const FDrivenMontages& DrivenMontages, float Duration, float Rate, const FName& StartSection);

	// Stop

	/** Performs same checks in UAbilitySystemComponent::CurrentMontageStop - this might be suboptimal */
	static bool CanStopCurrentMontage(const TWeakObjectPtr<UAbilitySystemComponent>& AbilitySystemComponent, const TObjectPtr<UGameplayAbility>& Ability);

	static void StopMontage(float BlendOutTime, const FDrivenMontagePair& Montage);

	static void StopDrivenMontages(const AActor* AvatarActor, const FDrivenMontages& DrivenMontages, float BlendOutTime);
};
