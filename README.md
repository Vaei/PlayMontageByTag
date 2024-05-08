# Play Montage By Tag

Gives you ability nodes to play montages by tag.

If you want to use blueprint, you need to add this manually by adding a `BlueprintImplementableEvent` that `IPlayMontageByTagInterface::GetAbilityMontagesByTag` calls.

## Setup

You will need to include PlayMontageByTag in both your .uproject and your `Build.cs` `PublicDependencyModuleNames`

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

### 1.0.0
* Release