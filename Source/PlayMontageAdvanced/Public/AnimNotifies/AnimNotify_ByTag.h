// Copyright (c) 2024 Studio Titan

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PlayMontageTags.h"
#include "PlayMontageAdvancedTypes.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ByTag.generated.h"

/**
 * Does nothing on its own
 * Parsed by PlayMontageAdvanced to provide a callback
 */
UCLASS()
class PLAYMONTAGEADVANCED_API UAnimNotify_ByTag : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** Tag broadcast by PlayMontageByTag for this notify */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotify)
	FGameplayTag NotifyTag = PlayMontageTags::MontageTag_Notify;

	/** Ensure that notifies are triggered if the montage aborts before they're reached when aborted due to these conditions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotify)
	TArray<EPlayMontageAdvancedEventType> EnsureTriggerNotify = {};

	UAnimNotify_ByTag()
	{
#if WITH_EDITORONLY_DATA
		bShouldFireInEditor = false;
		NotifyColor = FColor(65.f, 65.f, 65.f, 255.f);
#endif
	}

#if WITH_EDITOR
	virtual void OnAnimNotifyCreatedInEditor(FAnimNotifyEvent& ContainingAnimNotifyEvent) override
	{
		Super::OnAnimNotifyCreatedInEditor(ContainingAnimNotifyEvent);
		ContainingAnimNotifyEvent.bTriggerOnDedicatedServer = false;
		ContainingAnimNotifyEvent.NotifyTriggerChance = 0.f;  // Parsed, never triggers
	}
#endif
	
	virtual FString GetNotifyName_Implementation() const override
	{
		return "ByTag: " + NotifyTag.ToString();
	}
};
