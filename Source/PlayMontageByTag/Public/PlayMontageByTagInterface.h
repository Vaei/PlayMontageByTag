// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayMontageByTagInterface.generated.h"

struct FDrivenMontages;
struct FGameplayTag;

UINTERFACE()
class UPlayMontageByTagInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PLAYMONTAGEBYTAG_API IPlayMontageByTagInterface
{
	GENERATED_BODY()

public:
	virtual bool GetAbilityMontagesByTag(const FGameplayTag& MontageTag, UAnimMontage*& DriverMontage, FDrivenMontages& DrivenMontages) const PURE_VIRTUAL(, return false;);
};
