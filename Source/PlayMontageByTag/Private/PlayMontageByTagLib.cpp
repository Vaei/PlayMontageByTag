// Copyright (c) Jared Taylor. All Rights Reserved


#include "PlayMontageByTagLib.h"

#include "AbilitySystemComponent.h"
#include "PlayMontageByTagTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayMontageByTagLib)

float UPlayMontageByTagLib::GetMontagePlayRateScaledByDuration(const UAnimMontage* Montage, float Duration)
{
	if (Montage && Duration > 0.f)
	{
		return Montage->GetPlayLength() / Duration;
	}
	return 1.f;
}

bool UPlayMontageByTagLib::ShouldPlayLocalDrivenMontages(const AActor* AvatarActor)
{
	const APawn* Pawn = AvatarActor ? Cast<APawn>(AvatarActor) : nullptr;
	APlayerController* PlayerController = Pawn ? Pawn->GetLocalViewingPlayerController() : nullptr;
	return PlayerController != nullptr;
}

void UPlayMontageByTagLib::PlayDrivenMontage(float Duration, float Rate, const FName& StartSection,
	const FDrivenMontagePair& Montage)
{
	if (!Montage.Montage)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = Montage.SkeletalMeshComponent ?
		Montage.SkeletalMeshComponent->GetAnimInstance() : nullptr)
	{
		// Returns MontageLength which ignores Rate, so we must scale by Rate again
		const float ScaledDuration = GetMontagePlayRateScaledByDuration(Montage.Montage, Duration);
		AnimInstance->Montage_Play(Montage.Montage, Rate * ScaledDuration);

		if (StartSection != NAME_None)
		{
			AnimInstance->Montage_JumpToSection(StartSection, Montage.Montage);
		}
	}
}

void UPlayMontageByTagLib::PlayDrivenMontages(const AActor* AvatarActor, const FDrivenMontages& DrivenMontages,
	float Duration, float Rate, const FName& StartSection)
{
	for (const auto& Montage : DrivenMontages.DrivenMontages)
	{
		PlayDrivenMontage(Duration, Rate, StartSection, Montage);
	}

	// Only play local montages on local players or spectators
	if (!ShouldPlayLocalDrivenMontages(AvatarActor))
	{
		return;
	}
	
	for (const auto& Montage : DrivenMontages.LocalDrivenMontages)
	{
		PlayDrivenMontage(Duration, Rate, StartSection, Montage);
	}
}

bool UPlayMontageByTagLib::CanStopCurrentMontage(const TWeakObjectPtr<UAbilitySystemComponent>& AbilitySystemComponent,
	const TObjectPtr<UGameplayAbility>& Ability)
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	const FGameplayAbilityActorInfo* AbilityActorInfo = Ability->GetCurrentActorInfo();
	
	UAnimInstance* AnimInstance = AbilityActorInfo ? AbilityActorInfo->GetAnimInstance() : nullptr;
	UAnimMontage* MontageToStop = ASC->GetCurrentMontage();
	bool bShouldStopMontage = AnimInstance && MontageToStop && !AnimInstance->Montage_GetIsStopped(MontageToStop);

	return bShouldStopMontage;
}

void UPlayMontageByTagLib::StopMontage(float BlendOutTime, const FDrivenMontagePair& Montage)
{
	if (!Montage.Montage)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = Montage.SkeletalMeshComponent ?
		Montage.SkeletalMeshComponent->GetAnimInstance() : nullptr)
	{
		AnimInstance->Montage_Stop(BlendOutTime, Montage.Montage);
	}
}

void UPlayMontageByTagLib::StopDrivenMontages(const AActor* AvatarActor, const FDrivenMontages& DrivenMontages,
	float BlendOutTime)
{
	for (const auto& Montage : DrivenMontages.DrivenMontages)
	{
		StopMontage(BlendOutTime, Montage);
	}

	// Only play local montages on local players or spectators
	if (!ShouldPlayLocalDrivenMontages(AvatarActor))
	{
		return;
	}
	
	for (const auto& Montage : DrivenMontages.LocalDrivenMontages)
	{
		StopMontage(BlendOutTime, Montage);
	}
}
