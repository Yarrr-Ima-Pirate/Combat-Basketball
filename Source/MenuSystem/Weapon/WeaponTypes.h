#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_CatLauncher UMETA(DisplayName = "Cat Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
	
	EWT_MAX UMETA(DisplayName = "DefaultMax")
};
