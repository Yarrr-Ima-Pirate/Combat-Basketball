// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "MenuSystem/Character/ArenaBallCharacter.h"
#include "MenuSystem/ArenaComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AArenaBallCharacter* ArenaBallCharacter = Cast<AArenaBallCharacter>(OtherActor);
	if (ArenaBallCharacter)
	{
		UCombatComponent* Combat = ArenaBallCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();
}
