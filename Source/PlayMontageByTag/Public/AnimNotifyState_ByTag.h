// Copyright (c) 2024 Studio Titan

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PlayMontageByTagTags.h"
#include "PlayMontageByTagTypes.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ByTag.generated.h"

/**
 * Does nothing on its own
 * Parsed by PlayMontageByTag to provide a callback
 */
UCLASS()
class PLAYMONTAGEBYTAG_API UAnimNotifyState_ByTag : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Tag broadcast by PlayMontageByTag for this notify */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotify)
	FGameplayTag NotifyTag = PlayMontageTags::MontageTag_Notify;

	/** Ensure that notifies are triggered if the montage aborts before they're reached when aborted due to these conditions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotify)
	TArray<EPlayMontageByTagEventType> EnsureTriggerNotify = {};

	/** Ensure that the end state is reached if the notify was triggered */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotify)
	bool bEnsureEndStateIfTriggered = true;
	
	UAnimNotifyState_ByTag()
	{
#if WITH_EDITORONLY_DATA
		bShouldFireInEditor = false;
		NotifyColor = FColor(50.f, 50.f, 50.f, 255.f);
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
