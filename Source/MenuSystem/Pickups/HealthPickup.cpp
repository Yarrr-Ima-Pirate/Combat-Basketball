// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "MenuSystem/Character/ArenaBallCharacter.h"
#include "MenuSystem/ArenaComponents/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
	bReplicates = true;
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AArenaBallCharacter* ArenaBallCharacter = Cast<AArenaBallCharacter>(OtherActor);
	if (ArenaBallCharacter)
	{
		UBuffComponent* Buff = ArenaBallCharacter->GetBuff();
		if (Buff)
		{
			Buff->Heal(HealAmount, HealingTime);
		}
	}
	
	Destroy();
}