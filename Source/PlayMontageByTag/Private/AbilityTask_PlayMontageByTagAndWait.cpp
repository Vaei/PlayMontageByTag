// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilityTask_PlayMontageByTagAndWait.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "PlayMontageByTagInterface.h"
#include "PlayTagAbilitySystemComponent.h"
#include "PlayMontageByTagLib.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_PlayMontageByTagAndWait)

static bool GUseAggressivePlayMontageAndWaitEndTask = true;
static FAutoConsoleVariableRef CVarAggressivePlayMontageAndWaitEndTask(TEXT("AbilitySystem.PlayMontageByTag.AggressiveEndTask"), GUseAggressivePlayMontageAndWaitEndTask, TEXT("This should be set to true in order to avoid multiple callbacks off an AbilityTask_PlayMontageByTagAndWait node"));

void UAbilityTask_PlayMontageByTagAndWait::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
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

			if (GUseAggressivePlayMontageAndWaitEndTask)
			{
				EndTask();
			}
		}
		else
		{
			OnBlendOut.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
}

void UAbilityTask_PlayMontageByTagAndWait::OnGameplayAbilityCancelled()
{
	if (StopPlayingMontage(OverrideBlendOutTimeOnCancelAbility) || bAllowInterruptAfterBlendOut)
	{
		// Let the BP handle the interrupt as well
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	if (GUseAggressivePlayMontageAndWaitEndTask)
	{
		EndTask();
	}
}

void UAbilityTask_PlayMontageByTagAndWait::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	EndTask();
}

UAbilityTask_PlayMontageByTagAndWait* UAbilityTask_PlayMontageByTagAndWait::CreatePlayMontageByTagAndWaitProxy(
	UGameplayAbility* OwningAbility, FName TaskInstanceName, FGameplayTag MontageTag, FGameplayTagContainer EventTags,
	float Rate, FName StartSection, bool bStopWhenAbilityEnds, float AnimRootMotionTranslationScale,
	float StartTimeSeconds, bool bOverrideBlendIn, FMontageBlendSettings BlendInOverride,
	bool bAllowInterruptAfterBlendOut, float OverrideBlendOutTimeOnCancelAbility, float	OverrideBlendOutTimeOnEndAbility)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	AActor* AvatarActor = OwningAbility->GetAvatarActorFromActorInfo();
	if (!ensure(IsValid(AvatarActor)))
	{
		// @todo can it fail IsValid when they get destroyed?
		return nullptr;
	}

	if (!ensure(AvatarActor->Implements<UPlayMontageByTagInterface>()))
	{
		return nullptr;
	}

	IPlayMontageByTagInterface* Interface = CastChecked<IPlayMontageByTagInterface>(AvatarActor);

	UAnimMontage* MontageToPlay;
	FDrivenMontages DrivenMontages;
	const bool bValid = Interface->GetAbilityMontagesByTag(MontageTag, MontageToPlay, DrivenMontages);
	if (!bValid)
	{
		return nullptr;
	}

	UAbilityTask_PlayMontageByTagAndWait* MyObj = NewAbilityTask<UAbilityTask_PlayMontageByTagAndWait>(OwningAbility, TaskInstanceName);
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->EventTags = EventTags;
	MyObj->DrivenMontages = DrivenMontages;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	MyObj->bAllowInterruptAfterBlendOut = bAllowInterruptAfterBlendOut;
	MyObj->StartTimeSeconds = StartTimeSeconds;
	MyObj->bOverrideBlendIn = bOverrideBlendIn;
	MyObj->BlendInOverride = BlendInOverride;
	MyObj->OverrideBlendOutTimeOnCancelAbility = OverrideBlendOutTimeOnCancelAbility;
	MyObj->OverrideBlendOutTimeOnEndAbility = OverrideBlendOutTimeOnEndAbility;
	
	return MyObj;
}

float UAbilityTask_PlayMontageByTagAndWait::PlayDrivenMontageForMesh(UPlayTagAbilitySystemComponent* ASC, const float Duration, const FDrivenMontagePair& Montage, const bool bReplicate) const
{
	const float ScaledRate = Rate * UPlayMontageByTagLib::GetMontagePlayRateScaledByDuration(Montage.Montage, Duration);
	
	return ASC->PlayMontageForMesh(
		Ability, Montage.Mesh, Ability->GetCurrentActivationInfo(), Montage.Montage, ScaledRate,
		bOverrideBlendIn, BlendInOverride, StartSection, StartTimeSeconds, bReplicate);
}

void UAbilityTask_PlayMontageByTagAndWait::Activate()
{
	if (Ability == nullptr)
	{
		return;
	}

	bool bPlayedMontage = false;

	if (UPlayTagAbilitySystemComponent* ASC = AbilitySystemComponent.IsValid() ?
		Cast<UPlayTagAbilitySystemComponent>(AbilitySystemComponent.Get()) : nullptr)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			// Bind to event callback
			EventHandle = ASC->AddGameplayEventTagContainerDelegate(EventTags,
				FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnGameplayEvent));

			const float Duration = ASC->PlayMontageForMesh(Ability, ActorInfo->SkeletalMeshComponent.Get(),
				Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, bOverrideBlendIn, BlendInOverride,
				StartSection, StartTimeSeconds, true);
				
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

				InterruptedHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UAbilityTask_PlayMontageByTagAndWait::OnGameplayAbilityCancelled);

				BlendingOutDelegate.BindUObject(this, &UAbilityTask_PlayMontageByTagAndWait::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				MontageEndedDelegate.BindUObject(this, &UAbilityTask_PlayMontageByTagAndWait::OnMontageEnded);
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
			ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageByTagAndWait call to PlayMontage failed!"));
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageByTagAndWait called on invalid AbilitySystemComponent"));
	}

	if (!bPlayedMontage)
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageByTagAndWait called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay),*InstanceName.ToString());
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	SetWaitingOnAvatar();
}

void UAbilityTask_PlayMontageByTagAndWait::ExternalCancel()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
	}
	Super::ExternalCancel();
}

void UAbilityTask_PlayMontageByTagAndWait::OnDestroy(bool AbilityEnded)
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
		}
	}

	if (UPlayTagAbilitySystemComponent* ASC = AbilitySystemComponent.IsValid() ?
		Cast<UPlayTagAbilitySystemComponent>(AbilitySystemComponent.Get()) : nullptr)
	{
		ASC->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
	}

	Super::OnDestroy(AbilityEnded);

}

bool UAbilityTask_PlayMontageByTagAndWait::StopPlayingMontage(float OverrideBlendOutTime)
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
	UPlayTagAbilitySystemComponent* ASC = AbilitySystemComponent.IsValid() ?
		Cast<UPlayTagAbilitySystemComponent>(AbilitySystemComponent.Get()) : nullptr;
	
	if (ASC && Ability)
	{
		if (ASC->GetAnimatingAbility() == Ability
			&& ASC->GetCurrentMontage() == MontageToPlay)
		{
			// Unbind delegates so they don't get called as well
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay);
			if (MontageInstance)
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}
			
			for (const auto& Montage : DrivenMontages.DrivenMontages)
			{
				ASC->CurrentMontageStopForMesh(Montage.Mesh, OverrideBlendOutTime);
			}

			for (const auto& Montage : DrivenMontages.LocalDrivenMontages)
			{
				ASC->CurrentMontageStopForMesh(Montage.Mesh, OverrideBlendOutTime);
			}

			ASC->CurrentMontageStop(OverrideBlendOutTime);
			return true;
		}
	}

	return false;
}

void UAbilityTask_PlayMontageByTagAndWait::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempData = *Payload;
		TempData.EventTag = EventTag;

		OnEventReceived.Broadcast(EventTag, TempData);
	}
}

FString UAbilityTask_PlayMontageByTagAndWait::GetDebugString() const
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

