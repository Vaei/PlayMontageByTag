// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/AbilityTask_PlayMontageAdvanced.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "AnimNotifies/AnimNotifyState_ByTag.h"
#include "AnimNotifies/AnimNotify_ByTag.h"
#include "PlayMontageByTagInterface.h"
#include "AbilitySystem/PlayMontageAbilitySystemComponent.h"
#include "PlayMontageAdvancedLib.h"
#include "Tasks/GameplayTask_WaitDelay.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_PlayMontageAdvanced)

static bool GUseAggressivePlayMontageAndWaitEndTask = true;
static FAutoConsoleVariableRef CVarAggressivePlayMontageAndWaitEndTask(TEXT("AbilitySystem.PlayMontageAdvanced.AggressiveEndTask"), GUseAggressivePlayMontageAndWaitEndTask, TEXT("This should be set to true in order to avoid multiple callbacks off an AbilityTask_PlayMontageAdvancedAndWait node"));

#define LOCTEXT_NAMESPACE "PlayMontageAdvanced"

void UAbilityTask_PlayMontageAdvanced::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	const bool bPlayingThisMontage = (Montage == MontageToPlay) && Ability && Ability->GetCurrentMontage() == MontageToPlay;
	if (bPlayingThisMontage)
	{
		// Reset AnimRootMotionTranslationScale
		ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
		if (Character && (Character->GetLocalRole() == ROLE_Authority ||
							(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
		{
			Character->SetAnimRootMotionTranslationScale(1.f);
		}
	}

	if (bPlayingThisMontage && (bInterrupted || !bAllowInterruptAfterBlendOut))
	{
		if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
		{
			ASC->ClearAnimatingAbility(Ability);
		}
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bInterrupted)
		{
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
			EnsureBroadcastTagEvents(EPlayMontageAdvancedEventType::OnInterrupted);

			if (GUseAggressivePlayMontageAndWaitEndTask)
			{
				EndTask();
			}
		}
		else
		{
			OnBlendOut.Broadcast(FGameplayTag(), FGameplayEventData());
			EnsureBroadcastTagEvents(EPlayMontageAdvancedEventType::BlendOut);
		}
	}
}

void UAbilityTask_PlayMontageAdvanced::OnGameplayAbilityCancelled()
{
	if (StopPlayingMontage(OverrideBlendOutTimeOnCancelAbility) || bAllowInterruptAfterBlendOut)
	{
		// Let the BP handle the interrupt as well
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
			EnsureBroadcastTagEvents(EPlayMontageAdvancedEventType::OnInterrupted);
		}
	}

	if (GUseAggressivePlayMontageAndWaitEndTask)
	{
		EndTask();
	}
}

void UAbilityTask_PlayMontageAdvanced::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast(FGameplayTag(), FGameplayEventData());
			EnsureBroadcastTagEvents(EPlayMontageAdvancedEventType::OnCompleted);
		}
	}

	EndTask();
}

UAbilityTask_PlayMontageAdvanced* UAbilityTask_PlayMontageAdvanced::CreatePlayMontageAdvancedAndWaitProxy(
	UGameplayAbility* OwningAbility, FName TaskInstanceName, FMontageAdvancedParams InputParams,
	FGameplayTag MontageTag, FGameplayTagContainer EventTags, float Rate,
	FName StartSection, bool bStopWhenAbilityEnds, float AnimRootMotionTranslationScale, float StartTimeSeconds,
	EPlayMontageAdvancedNotifyHandling NotifyHandling, bool bTriggerNotifiesBeforeStartTimeSeconds,
	bool bDrivenMontagesMatchDriverDuration, bool bOverrideBlendIn, FMontageBlendSettings BlendInOverride,
	bool bAllowInterruptAfterBlendOut, float OverrideBlendOutTimeOnCancelAbility,
	float OverrideBlendOutTimeOnEndAbility)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	AActor* AvatarActor = OwningAbility->GetAvatarActorFromActorInfo();
	if (!ensure(IsValid(AvatarActor)))
	{
		// @todo can it fail IsValid when they get destroyed?
		return nullptr;
	}

	FMontageAdvancedParams MontageParams = InputParams;
	if (!MontageParams.ParamsUsed())
	{
		if (!ensure(AvatarActor->Implements<UPlayMontageByTagInterface>()))
		{
#if !UE_BUILD_SHIPPING
			FMessageLog("PIE").Error(FText::Format(LOCTEXT("PlayMontageAdvanced_NoInterface",
				"UAbilityTask_PlayMontageAdvanced: Avatar actor {0} does not implement IPlayMontageByTagInterface and no InputParams were passed -- one or the other must be available"), FText::FromString(AvatarActor->GetName())));
#endif
			return nullptr;
		}

		IPlayMontageByTagInterface* Interface = CastChecked<IPlayMontageByTagInterface>(AvatarActor);
		const bool bValid = Interface->GetAbilityMontagesByTag(MontageTag, MontageParams);
		if (!bValid)
		{
			return nullptr;
		}
	}
	
	UAbilityTask_PlayMontageAdvanced* MyObj = NewAbilityTask<UAbilityTask_PlayMontageAdvanced>(OwningAbility, TaskInstanceName);
	MyObj->MontageToPlay = MontageParams.DriverMontage;
	MyObj->EventTags = EventTags;
	MyObj->DrivenMontages = MontageParams.DrivenMontages;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	MyObj->bAllowInterruptAfterBlendOut = bAllowInterruptAfterBlendOut;
	MyObj->StartTimeSeconds = StartTimeSeconds;
	MyObj->NotifyHandling = NotifyHandling;
	MyObj->bTriggerNotifiesBeforeStartTimeSeconds = bTriggerNotifiesBeforeStartTimeSeconds;
	MyObj->bDrivenMontagesMatchDriverDuration = bDrivenMontagesMatchDriverDuration;
	MyObj->bOverrideBlendIn = bOverrideBlendIn;
	MyObj->BlendInOverride = BlendInOverride;
	MyObj->OverrideBlendOutTimeOnCancelAbility = OverrideBlendOutTimeOnCancelAbility;
	MyObj->OverrideBlendOutTimeOnEndAbility = OverrideBlendOutTimeOnEndAbility;
	
	return MyObj;
}

float UAbilityTask_PlayMontageAdvanced::PlayDrivenMontageForMesh(UPlayMontageAbilitySystemComponent* ASC, const float Duration, const FDrivenMontagePair& Montage, const bool bReplicate) const
{
	const float ScaledRate = bDrivenMontagesMatchDriverDuration ?
		Rate * UPlayMontageAdvancedLib::GetMontagePlayRateScaledByDuration(Montage.Montage, Duration)
		: Rate;
	
	return ASC->PlayMontageForMesh(
		Ability, Montage.Mesh, Ability->GetCurrentActivationInfo(), Montage.Montage, ScaledRate,
		bOverrideBlendIn, BlendInOverride, StartSection, StartTimeSeconds, bReplicate);
}

void UAbilityTask_PlayMontageAdvanced::Activate()
{
	if (Ability == nullptr)
	{
		return;
	}

	bool bPlayedMontage = false;

	if (UPlayMontageAbilitySystemComponent* ASC = AbilitySystemComponent.IsValid() ?
		Cast<UPlayMontageAbilitySystemComponent>(AbilitySystemComponent.Get()) : nullptr)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			// Bind to event callback
			EventHandle = ASC->AddGameplayEventTagContainerDelegate(EventTags,
				FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnGameplayEvent));

			// Gather notifies with tags
			NotifyByTags.Reset();
			switch (NotifyHandling)
			{
			case EPlayMontageAdvancedNotifyHandling::Montage:
				{
					// Gather notifies from driver montage
					TArray<FAnimNotifyEvent>& Notifies = MontageToPlay->Notifies;
					for (FAnimNotifyEvent& Notify : Notifies)
					{
						const float StartTime = Notify.GetTime();
						
						if (UAnimNotify_ByTag* NotifyByTag = Notify.Notify ? Cast<UAnimNotify_ByTag>(Notify.Notify) : nullptr)
						{
							FAnimNotifyByTagEvent NotifyByTagEvent = { NotifyByTag->NotifyTag, NotifyByTag->EnsureTriggerNotify, 
							EPlayMontageAdvancedNotifyType::Notify, Notify.GetTime() };
							NotifyByTags.Add(NotifyByTagEvent);
						}

						if (UAnimNotifyState_ByTag* NotifyStateByTag = Notify.NotifyStateClass ? Cast<UAnimNotifyState_ByTag>(Notify.NotifyStateClass) : nullptr)
						{
							const float EndTime = StartTime + Notify.GetDuration();

							// Start state notify
							FAnimNotifyByTagEvent& NotifyByTagEvent = NotifyByTags.Add_GetRef({
								NotifyStateByTag->NotifyTag, NotifyStateByTag->EnsureTriggerNotify,
								EPlayMontageAdvancedNotifyType::NotifyStateBegin, Notify.GetTime() });

							// End state notify
							FAnimNotifyByTagEvent& NotifyStateEndByTagEvent = NotifyByTags.Add_GetRef({
								NotifyStateByTag->NotifyTag, NotifyStateByTag->EnsureTriggerNotify,
								EPlayMontageAdvancedNotifyType::NotifyStateEnd, EndTime });

							NotifyStateEndByTagEvent.bIsEndState = true;

							// Pair begin and end states
							NotifyByTagEvent.NotifyStatePair = &NotifyStateEndByTagEvent;
							NotifyStateEndByTagEvent.NotifyStatePair = &NotifyByTagEvent;
						}
					}
				}
				break;
			// Note: Could add enum option to grab all anim sequences and parse those too
			case EPlayMontageAdvancedNotifyHandling::Disabled:
				break;
			}

			// Trigger notifies before start time and remove them, if we want to trigger them before the start time
			// Otherwise remove them without triggering
			for (FAnimNotifyByTagEvent& TagEvent : NotifyByTags)
			{
				if (TagEvent.Time <= StartTimeSeconds)
				{
					if (bTriggerNotifiesBeforeStartTimeSeconds)
					{
						BroadcastTagEvent(TagEvent);
					}
					else
					{
						TagEvent.bNotifySkipped = true;
					}
				}
			}
			
			// Create tasks for notifies
			UWorld* World = GetWorld();
			for (FAnimNotifyByTagEvent& TagEvent : NotifyByTags)
			{
				// Set up timer for notify
				TagEvent.TimerDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::OnTimer, &TagEvent);
				World->GetTimerManager().SetTimer(TagEvent.Timer, TagEvent.TimerDelegate, TagEvent.Time, false);
			}
			
			// Play Driver Montage
			const float Duration = ASC->PlayMontageForMesh(Ability, ActorInfo->SkeletalMeshComponent.Get(),
				Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, bOverrideBlendIn, BlendInOverride,
				StartSection, StartTimeSeconds, true);

			// Play Driven Montages
			if (Duration > 0.f)
			{
				for (const auto& Montage : DrivenMontages.DrivenMontages)
				{
					constexpr bool bReplicate = true;
					PlayDrivenMontageForMesh(ASC, Duration, Montage, bReplicate);
				}

				for (const auto& Montage : DrivenMontages.LocalDrivenMontages)
				{
					constexpr bool bReplicate = false;
					PlayDrivenMontageForMesh(ASC, Duration, Montage, bReplicate);
				}

				// Playing a montage could potentially fire off a callback into game code which could kill this ability! Early out if we are  pending kill.
				if (ShouldBroadcastAbilityTaskDelegates() == false)
				{
					return;
				}

				InterruptedHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UAbilityTask_PlayMontageAdvanced::OnGameplayAbilityCancelled);

				BlendingOutDelegate.BindUObject(this, &UAbilityTask_PlayMontageAdvanced::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				MontageEndedDelegate.BindUObject(this, &UAbilityTask_PlayMontageAdvanced::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
				if (Character && (Character->GetLocalRole() == ROLE_Authority ||
								  (Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
				{
					Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
				}

				bPlayedMontage = true;
			}
		}
		else
		{
			ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageAdvanced call to PlayMontage failed!"));
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageAdvanced called on invalid AbilitySystemComponent"));
	}

	if (!bPlayedMontage)
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageAdvanced called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay),*InstanceName.ToString());
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());

			// Don't call ensure broadcast tag events here, as we didn't play the montage
		}
	}

	SetWaitingOnAvatar();
}

void UAbilityTask_PlayMontageAdvanced::ExternalCancel()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		EnsureBroadcastTagEvents(EPlayMontageAdvancedEventType::OnCancelled);
	}
	Super::ExternalCancel();
}

void UAbilityTask_PlayMontageAdvanced::OnDestroy(bool AbilityEnded)
{
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	// This delegate, however, should be cleared as it is a multicast
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(InterruptedHandle);
		if (AbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage(OverrideBlendOutTimeOnEndAbility);
			
			// Let the BP handle the interrupt as well
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
			EnsureBroadcastTagEvents(EPlayMontageAdvancedEventType::OnInterrupted);
		}
	}

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.IsValid() ?
		Cast<UAbilitySystemComponent>(AbilitySystemComponent.Get()) : nullptr)
	{
		ASC->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
	}

	Super::OnDestroy(AbilityEnded);

}

bool UAbilityTask_PlayMontageAdvanced::StopPlayingMontage(float OverrideBlendOutTime)
{
	if (Ability == nullptr)
	{
		return false;
	}

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (ActorInfo == nullptr)
	{
		return false;
	}

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		return false;
	}

	// Check if the montage is still playing
	// The ability would have been interrupted, in which case we should automatically stop the montage
	UPlayMontageAbilitySystemComponent* ASC = AbilitySystemComponent.IsValid() ?
		Cast<UPlayMontageAbilitySystemComponent>(AbilitySystemComponent.Get()) : nullptr;

	USkeletalMeshComponent* Mesh = ActorInfo && ActorInfo->SkeletalMeshComponent.IsValid() ? ActorInfo->SkeletalMeshComponent.Get() : nullptr;
	
	if (ASC && Ability && Mesh)
	{
		if (ASC->GetAnimatingAbilityFromMesh(Mesh) == Ability
			&& ASC->GetCurrentMontageForMesh(Mesh) == MontageToPlay)
		{
			// Unbind delegates so they don't get called as well
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay);
			if (MontageInstance)
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}

			// Driver Montage
			ASC->CurrentMontageStopForMesh(Mesh, OverrideBlendOutTime);
			
			// Driven Montages
			for (const auto& Montage : DrivenMontages.DrivenMontages)
			{
				ASC->CurrentMontageStopForMesh(Montage.Mesh, OverrideBlendOutTime);
			}

			// Local Driven Montages
			for (const auto& Montage : DrivenMontages.LocalDrivenMontages)
			{
				ASC->CurrentMontageStopForMesh(Montage.Mesh, OverrideBlendOutTime);
			}
		}
	}

	return false;
}

void UAbilityTask_PlayMontageAdvanced::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempData = *Payload;
		TempData.EventTag = EventTag;

		OnEventReceived.Broadcast(EventTag, TempData);
	}
}

void UAbilityTask_PlayMontageAdvanced::BroadcastTagEvent(FAnimNotifyByTagEvent& TagEvent) const
{
	// Ensure we don't broadcast the same event twice
	if (TagEvent.bHasBroadcast || TagEvent.bNotifySkipped)
	{
		return;
	}

	// Ensure the start state broadcasts first if this is the end state
	if (TagEvent.bIsEndState && TagEvent.NotifyStatePair)
	{
		// If our start state was skipped, we can't broadcast the end state
		if (TagEvent.NotifyStatePair->bNotifySkipped)
		{
			return;
		}

		// Broadcast the start state first
		if (!TagEvent.NotifyStatePair->bHasBroadcast)
		{
			BroadcastTagEvent(*TagEvent.NotifyStatePair);
		}
	}

	// Mark the event as broadcast and clear timers
	TagEvent.bHasBroadcast = true;
	TagEvent.ClearTimers();

	// Broadcast the notify
	switch (TagEvent.NotifyType)
	{
	case EPlayMontageAdvancedNotifyType::Notify:
		OnNotify.Broadcast(TagEvent.Tag, FGameplayEventData());
		break;
	case EPlayMontageAdvancedNotifyType::NotifyStateBegin:
		OnNotifyStateBegin.Broadcast(TagEvent.Tag, FGameplayEventData());
		break;
	case EPlayMontageAdvancedNotifyType::NotifyStateEnd:
		OnNotifyStateEnd.Broadcast(TagEvent.Tag, FGameplayEventData());						
		break;
	}
}

void UAbilityTask_PlayMontageAdvanced::EnsureBroadcastTagEvents(EPlayMontageAdvancedEventType EventType)
{
	for (FAnimNotifyByTagEvent& TagEvent : NotifyByTags)
	{
		if (TagEvent.bHasBroadcast)
		{
			continue;
		}
		
		// Ensure that notifies are triggered if the montage aborts before they're reached when aborted due to these conditions
		if (TagEvent.EnsureTriggerNotify.Contains(EventType))
		{
			BroadcastTagEvent(TagEvent);
		}
		
		// Ensure that the end state is reached if the start state notify was triggered
		if (TagEvent.bIsEndState && TagEvent.NotifyStatePair && TagEvent.NotifyStatePair->bHasBroadcast)
		{
			BroadcastTagEvent(TagEvent);
		}
	}
}

void UAbilityTask_PlayMontageAdvanced::OnTimer(FAnimNotifyByTagEvent* TagEvent)
{
	if (TagEvent)
	{
		BroadcastTagEvent(*TagEvent);
	}
}

FString UAbilityTask_PlayMontageAdvanced::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();

		if (AnimInstance != nullptr)
		{
			PlayingMontage = AnimInstance->Montage_IsActive(MontageToPlay) ? ToRawPtr(MontageToPlay) : AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWait. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}

#undef LOCTEXT_NAMESPACE