// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "PlayMontageByTagTypes.h"
#include "PlayTagAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Animation/AnimInstance.h"
#include "AbilityTask_PlayMontageByTagAndWait.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMontageTagWaitDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMontageTagWaitEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

/** Ability task to simply play a montage. Many games will want to make a modified version of this task that looks for game-specific events */
UCLASS()
class PLAYMONTAGEBYTAG_API UAbilityTask_PlayMontageByTagAndWait : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable)
	FMontageTagWaitEventDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FMontageTagWaitEventDelegate OnBlendOut;

	UPROPERTY(BlueprintAssignable)
	FMontageTagWaitEventDelegate OnInterrupted;

	UPROPERTY(BlueprintAssignable)
	FMontageTagWaitEventDelegate OnCancelled;

	UPROPERTY(BlueprintAssignable)
	FMontageTagWaitEventDelegate OnEventReceived;

	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	/** Callback function for when the owning Gameplay Ability is cancelled */
	UFUNCTION()
	void OnGameplayAbilityCancelled();

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** 
	 * Start playing an animation montage on the avatar actor and wait for it to finish
	 * If StopWhenAbilityEnds is true, this montage will be aborted if the ability ends normally. It is always stopped when the ability is explicitly cancelled.
	 * On normal execution, OnBlendOut is called when the montage is blending out, and OnCompleted when it is completely done playing
	 * OnInterrupted is called if another montage overwrites this, and OnCancelled is called if the ability or task is cancelled
	 *
	 * @param TaskInstanceName Set to override the name of this task, for later querying
	 * @param MontageTag The tag to find montages for
	 * @param EventTags Any gameplay events matching this tag will activate the EventReceived callback. If empty, all events will trigger callback
	 * @param Rate Change to play the montage faster or slower
	 * @param StartSection If not empty, named montage section to start from
	 * @param bStopWhenAbilityEnds If true, this montage will be aborted if the ability ends normally. It is always stopped when the ability is explicitly cancelled
	 * @param AnimRootMotionTranslationScale Change to modify size of root motion or set to 0 to block it entirely
	 * @param bOverrideBlendIn If true apply BlendInOverride settings instead of the settings assigned to the montage
	 * @param StartTimeSeconds Starting time offset in montage, this will be overridden by StartSection if that is also set
	 * @param bDrivenMontagesMatchDriverDuration If true, all driven montages will run for the same duration as the driver montage
	 * @param bAllowInterruptAfterBlendOut If true, you can receive OnInterrupted after an OnBlendOut started (otherwise OnInterrupted will not fire when interrupted, but you will not get OnComplete).
	 * @param OverrideBlendOutTimeOnCancelAbility If >= 0 it will override the blend out time when ability is cancelled.
	 * @param OverrideBlendOutTimeOnEndAbility If >= 0 it will override the blend out time when ability ends.
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (DisplayName="PlayMontageByTagAndWait",
		HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE", MontageTag="MontageTag"))
	static UAbilityTask_PlayMontageByTagAndWait* CreatePlayMontageByTagAndWaitProxy(UGameplayAbility* OwningAbility,
		FName TaskInstanceName, FGameplayTag MontageTag, FGameplayTagContainer EventTags, float Rate = 1.f, 
		FName StartSection = NAME_None, bool bStopWhenAbilityEnds = true, float AnimRootMotionTranslationScale = 1.f,
		float StartTimeSeconds = 0.f, bool bDrivenMontagesMatchDriverDuration = true, bool bOverrideBlendIn = false, FMontageBlendSettings BlendInOverride = FMontageBlendSettings(), bool bAllowInterruptAfterBlendOut = false,
		float OverrideBlendOutTimeOnCancelAbility = -1.f, float OverrideBlendOutTimeOnEndAbility = -1.f);

	float PlayDrivenMontageForMesh(UPlayTagAbilitySystemComponent* ASC, float Duration,
		const FDrivenMontagePair& Montage, bool bReplicate) const;

	virtual void Activate() override;

	/** Called when the ability is asked to cancel from an outside node. What this means depends on the individual task. By default, this does nothing other than ending the task. */
	virtual void ExternalCancel() override;

	virtual FString GetDebugString() const override;

protected:
	virtual void OnDestroy(bool AbilityEnded) override;

	/** Checks if the ability is playing a montage and stops that montage, returns true if a montage was stopped, false if not. */
	bool StopPlayingMontage(float OverrideBlendOutTime = -1.f);

	void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle InterruptedHandle;

	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY()
	FGameplayTagContainer EventTags;

	UPROPERTY()
	FDrivenMontages DrivenMontages;

	UPROPERTY()
	float Rate;

	UPROPERTY()
	FName StartSection;

	UPROPERTY()
	float AnimRootMotionTranslationScale;

	UPROPERTY()
	float StartTimeSeconds;

	UPROPERTY()
	bool bDrivenMontagesMatchDriverDuration;
	
	UPROPERTY()
	bool bOverrideBlendIn;
	
	UPROPERTY()
	FMontageBlendSettings BlendInOverride;

	UPROPERTY()
	bool bStopWhenAbilityEnds;

	UPROPERTY()
	bool bAllowInterruptAfterBlendOut;
	
	UPROPERTY()
	float OverrideBlendOutTimeOnCancelAbility;

	UPROPERTY()
	float OverrideBlendOutTimeOnEndAbility;

	FDelegateHandle EventHandle;
};
