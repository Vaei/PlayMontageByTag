// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayMontageAdvancedTypes.h"
#include "PlayMontageByTagInterface.generated.h"

struct FGameplayTag;

UINTERFACE()
class UPlayMontageByTagInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PLAYMONTAGEADVANCED_API IPlayMontageByTagInterface
{
	GENERATED_BODY()

public:
	virtual bool GetAbilityMontagesByTag(const FGameplayTag& MontageTag, FMontageAdvancedParams& MontageParams) const PURE_VIRTUAL(, return false;);
};
