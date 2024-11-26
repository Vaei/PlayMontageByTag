// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "PlayMontageAdvancedTypes.h"
#include "AbilitySystem/PlayMontageAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Animation/AnimInstance.h"
#include "AbilityTask_PlayMontageAdvanced.generated.h"

class UGameplayTask_WaitDelay;
struct FMontageBlendSettings;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMontageTagWaitDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMontageTagWaitEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

/**
 * Handling for parsing notifies in the montage to be triggered by tags
 */
UENUM(BlueprintType)
enum class EPlayMontageAdvancedNotifyHandling : uint8
{
	Montage			UMETA(ToolTip="Driver montage will be checked for notifies"),
	Disabled		UMETA(ToolTip="Notifies will not be handled"),
};

enum class EPlayMontageByTagNotifyType : uint8
{
	Notify,
	NotifyStateBegin,
	NotifyStateEnd,
};

USTRUCT()
struct FAnimNotifyByTagEvent
{
	GENERATED_BODY()
	
	FAnimNotifyByTagEvent(const FGameplayTag& InTag = FGameplayTag::EmptyTag, const TArray<EPlayMontageAdvancedEventType>& bInEnsureTriggerNotify = {}, EPlayMontageByTagNotifyType InNotifyType = EPlayMontageByTagNotifyType::Notify, float InTime = 0.f)
		: Tag(InTag)
		, EnsureTriggerNotify(bInEnsureTriggerNotify)
		, bEnsureEndStateIfTriggered(true)
		, Time(InTime)
		, bHasBroadcast(false)
		, bIsEndState(false)
		, bNotifySkipped(false)
		, NotifyStatePair(nullptr)
		, NotifyType(InNotifyType)
	{}

	UPROPERTY()
	FGameplayTag Tag;
	
	UPROPERTY()
	TArray<EPlayMontageAdvancedEventType> EnsureTriggerNotify;

	UPROPERTY()
	bool bEnsureEndStateIfTriggered;

	UPROPERTY()
	float Time;

	UPROPERTY()
	FGuid NotifyID;

	UPROPERTY()
	bool bHasBroadcast;

	UPROPERTY()
	bool bIsEndState;

	UPROPERTY()
	bool bNotifySkipped;
	
	FAnimNotifyByTagEvent* NotifyStatePair;

	EPlayMontageByTagNotifyType NotifyType;

	FTimerHandle Timer;
	FTimerDelegate TimerDelegate;

	void OnTimer();
	
	void ClearTimers()
	{
		if (Timer.IsValid())
		{
			Timer.Invalidate();
		}
		if (TimerDelegate.IsBound())
		{
			TimerDelegate.Unbind();
		}
	}

	bool IsValid() const { return Tag.IsValid() && NotifyID.IsValid(); }

	bool operator==(const FAnimNotifyByTagEvent& Other) const
	{
		return NotifyID.IsValid() && NotifyID == Other.NotifyID;
	}

	bool operator!=(const FAnimNotifyByTagEvent& Other) const
	{
		return !(*this == Other);
	}
};

/** Ability task to simply play a montage. Many games will want to make a modified version of this task that looks for game-specific events */
UCLASS()
class PLAYMONTAGEADVANCED_API UAbilityTask_PlayMontageAdvanced : public UAbilityTask
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

	UPROPERTY(BlueprintAssignable)
	FMontageTagWaitEventDelegate OnNotify;

	UPROPERTY(BlueprintAssignable)
	FMontageTagWaitEventDelegate OnNotifyStateBegin;

	UPROPERTY(BlueprintAssignable)
	FMontageTagWaitEventDelegate OnNotifyStateEnd;
	
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
	 * @param OwningAbility The ability that owns this task
	 * @param TaskInstanceName Set to override the name of this task, for later querying
	 * @param InputParams [OPTIONAL] The parameters to use for montage selection, if InputType is set to Parameters
	 * @param MontageTag [OPTIONAL] The tag to find montages for, if InputType is set to Interface
	 * @param EventTags Any gameplay events matching this tag will activate the EventReceived callback. If empty, all events will trigger callback
	 * @param Rate Change to play the montage faster or slower
	 * @param StartSection If not empty, named montage section to start from
	 * @param bStopWhenAbilityEnds If true, this montage will be aborted if the ability ends normally. It is always stopped when the ability is explicitly cancelled
	 * @param AnimRootMotionTranslationScale Change to modify size of root motion or set to 0 to block it entirely
	 * @param bOverrideBlendIn If true apply BlendInOverride settings instead of the settings assigned to the montage
	 * @param BlendInOverride Settings to use if bOverrideBlendIn is true
	 * @param StartTimeSeconds Starting time offset in montage, this will be overridden by StartSection if that is also set
	 * @param NotifyHandling How to handle 'by tag' notifies in the montage
	 * @param bTriggerNotifiesBeforeStartTimeSeconds If true, notifies clipped by StartTimeSeconds will be triggered even if they are before the start time
	 * @param bDrivenMontagesMatchDriverDuration If true, all driven montages will run for the same duration as the driver montage
	 * @param bAllowInterruptAfterBlendOut If true, you can receive OnInterrupted after an OnBlendOut started (otherwise OnInterrupted will not fire when interrupted, but you will not get OnComplete).
	 * @param OverrideBlendOutTimeOnCancelAbility If >= 0 it will override the blend out time when ability is cancelled.
	 * @param OverrideBlendOutTimeOnEndAbility If >= 0 it will override the blend out time when ability ends.
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (DisplayName="PlayMontageAdvancedAndWait",
		HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE", MontageTag="MontageTag"))
	static UAbilityTask_PlayMontageAdvanced* CreatePlayMontageAdvancedAndWaitProxy(UGameplayAbility* OwningAbility,
		FName TaskInstanceName, FMontageAdvancedParams InputParams, FGameplayTag MontageTag,
		FGameplayTagContainer EventTags, float Rate = 1.f, FName StartSection = NAME_None, bool bStopWhenAbilityEnds = true,
		float AnimRootMotionTranslationScale = 1.f, float StartTimeSeconds = 0.f,
		EPlayMontageAdvancedNotifyHandling NotifyHandling = EPlayMontageAdvancedNotifyHandling::Montage,
		bool bTriggerNotifiesBeforeStartTimeSeconds = true, bool bDrivenMontagesMatchDriverDuration = true,
		bool bOverrideBlendIn = false, FMontageBlendSettings BlendInOverride = FMontageBlendSettings(),
		bool bAllowInterruptAfterBlendOut = false, float OverrideBlendOutTimeOnCancelAbility = -1.f,
		float OverrideBlendOutTimeOnEndAbility = -1.f);

	float PlayDrivenMontageForMesh(UPlayMontageAbilitySystemComponent* ASC, float Duration,
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
	
	void BroadcastTagEvent(FAnimNotifyByTagEvent& TagEvent) const;
	
	void EnsureBroadcastTagEvents(EPlayMontageAdvancedEventType EventType);

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
	bool bTriggerNotifiesBeforeStartTimeSeconds;

	UPROPERTY()
	EPlayMontageAdvancedNotifyHandling NotifyHandling;

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

	UPROPERTY()
	TArray<FAnimNotifyByTagEvent> NotifyByTags;

	FDelegateHandle EventHandle;

	void OnTimer(FAnimNotifyByTagEvent* TagEvent);
};
