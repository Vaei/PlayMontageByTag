// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "PlayMontageAdvancedTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayMontageAdvancedEventType : uint8
{
	OnCompleted,
	BlendOut,
	OnInterrupted,
	OnCancelled,
};

USTRUCT(BlueprintType)
struct PLAYMONTAGEADVANCED_API FDrivenMontagePair
{
	GENERATED_BODY()

	FDrivenMontagePair(UAnimMontage* InMontage = nullptr, USkeletalMeshComponent* InSkeletalMeshComponent = nullptr)
		: Montage(InMontage)
		, Mesh(InSkeletalMeshComponent)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Montage)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Montage)
	TObjectPtr<USkeletalMeshComponent> Mesh;
};

USTRUCT(BlueprintType)
struct PLAYMONTAGEADVANCED_API FDrivenMontages
{
	GENERATED_BODY()

	FDrivenMontages()
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Montage)
	TArray<FDrivenMontagePair> DrivenMontages;

	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadWrite, Category=Montage)
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

USTRUCT(BlueprintType)
struct PLAYMONTAGEADVANCED_API FMontageAdvancedParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Montage)
	UAnimMontage* DriverMontage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Montage)
	FDrivenMontages DrivenMontages;

	bool ParamsUsed() const
	{
		return DriverMontage != nullptr || DrivenMontages.DrivenMontages.Num() > 0 || DrivenMontages.LocalDrivenMontages.Num() > 0;
	}
};