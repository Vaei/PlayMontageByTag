# Play Montage By Tag

Provides the ability task node `Play Montage by Tag and Wait` to play multiple montages on multiple meshes using only an `FGameplayTag` and `IPlayMontageByTagInterface::GetAbilityMontagesByTag`.

Supports:
 * Montage Replication
 * Event Tags
 * Override blend-in settings programmatically
 * Driver and Driven montages (Driven montages can match the animation duration of the Driver montage)

If you want to use blueprint, you need to add this manually by adding a `BlueprintImplementableEvent` that `IPlayMontageByTagInterface::GetAbilityMontagesByTag` calls, otherwise C++ is required.

![node](https://github.com/Vaei/repo_files/blob/main/PlayerMoveTo/gameplay_playermoveto.png)

## Setup

### Project

You will need to include `"PlayMontageByTag"` in both your .uproject and your `Build.cs` `PublicDependencyModuleNames`


### Ability System

Either use `UPlayTagAbilitySystemComponent` or derive your own `UAbilitySystemComponent` from it.

Derive your gameplay abilities from `UPlayTagGameplayAbility`.

### Actor

Implement the `IPlayMontageByTagInterface` interface to your Avatar Actor

Override `GetAbilityMontagesByTag`:

```cpp
bool AMyCharacterPlayer::GetAbilityMontagesByTag(const FGameplayTag& MontageTag, UAnimMontage*& DriverMontage,
	FDrivenMontages& DrivenMontages) const
{
	DriverMontage = ThirdPersonBodyMontage;
	DrivenMontages.Reset();

	// These montages will play for all
	DrivenMontages.DrivenMontages.Add({ ThirdPersonWeaponMontage, WeaponMeshTP });

	// These montages only play locally, as per UPlayMontageByTagLib::ShouldPlayLocalDrivenMontages()
	DrivenMontages.LocalDrivenMontages.Add({ FirstPersonBodyMontage, BodyMeshFP }, { FirstPersonWeaponMontage, WeaponMeshFP });
	
	// Returning false means no montage will play at all, it will abort
	return true;
}
```

Any montages added to `LocalDrivenMontages` will only play if this is true:

```cpp
bool UPlayMontageByTagLib::ShouldPlayLocalDrivenMontages(const AActor* AvatarActor)
{
	const APawn* Pawn = AvatarActor ? Cast<APawn>(AvatarActor) : nullptr;
	APlayerController* PlayerController = Pawn ? Pawn->GetLocalViewingPlayerController() : nullptr;
	return PlayerController != nullptr;
}
```

Now you can add a `PlayMontageByTagAndWait` node to your ability blueprint and it should work.

Your next step will be to pass in a `FGameplayTag` for `MontageTag` and factor that in for `GetAbilityMontagesByTag`. For example `MontageTag.Weapon.SMG.Reload`.

## Changelog

### 1.1.0
* Added driven montage replication
* Added blend in overrides
* Added event tag callback
* Added option to scale driven montages to driver duration

### 1.0.0
* Release