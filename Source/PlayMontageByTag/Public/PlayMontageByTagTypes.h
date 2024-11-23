// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "PlayMontageByTagTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayMontageByTagEventType : uint8
{
	OnCompleted,
	BlendOut,
	OnInterrupted,
	OnCancelled,
};

USTRUCT()
struct PLAYMONTAGEBYTAG_API FDrivenMontagePair
{
	GENERATED_BODY()

	FDrivenMontagePair(UAnimMontage* InMontage = nullptr, USkeletalMeshComponent* InSkeletalMeshComponent = nullptr)
		: Montage(InMontage)
		, Mesh(InSkeletalMeshComponent)
	{}

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh;
};

USTRUCT()
struct PLAYMONTAGEBYTAG_API FDrivenMontages
{
	GENERATED_BODY()

	FDrivenMontages()
	{}

	UPROPERTY()
	TArray<FDrivenMontagePair> DrivenMontages;

	UPROPERTY(NotReplicated)
	TArray<FDrivenMontagePair> LocalDrivenMontages;

	void Empty()
	{
		DrivenMontages.Empty();
		LocalDrivenMontages.Empty();
	}
	
	void Reset()
	{
		DrivenMontages.Reset();
		LocalDrivenMontages.Reset();
	}
};
