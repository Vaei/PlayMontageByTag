// Copyright (c) Jared Taylor. All Rights Reserved


#include "PlayTagGameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayTagGameplayAbility)

// Most of this is from GASShooter and therefore also Copyright 2024 Dan Kestranek.
// https://github.com/tranek/GASShooter


bool UPlayTagGameplayAbility::FindAbilityMeshMontage(USkeletalMeshComponent* InMesh,
	FAbilityMeshMontage& InAbilityMontage)
{
	for (FAbilityMeshMontage& MeshMontage : CurrentAbilityMeshMontages)
	{
		if (MeshMontage.Mesh == InMesh)
		{
			InAbilityMontage = MeshMontage;
			return true;
		}
	}

	return false;
}

UAnimMontage* UPlayTagGameplayAbility::GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh)
{
	FAbilityMeshMontage AbilityMeshMontage;
	if (FindAbilityMeshMontage(InMesh, AbilityMeshMontage))
	{
		return AbilityMeshMontage.Montage;
	}

	return nullptr;
}

void UPlayTagGameplayAbility::SetCurrentMontageForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* InCurrentMontage)
{
	ensure(IsInstantiated());

	FAbilityMeshMontage AbilityMeshMontage;
	if (FindAbilityMeshMontage(InMesh, AbilityMeshMontage))
	{
		AbilityMeshMontage.Montage = InCurrentMontage;
	}
	else
	{
		CurrentAbilityMeshMontages.Add(FAbilityMeshMontage(InMesh, InCurrentMontage));
	}
}
