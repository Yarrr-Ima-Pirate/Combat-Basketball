// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "BasketballWeapon.generated.h"

/**
 * 
 */
UCLASS()
class MENUSYSTEM_API ABasketballWeapon : public AWeapon
{
	GENERATED_BODY()
public:
	ABasketballWeapon();

protected:
	


	


private:
	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;	
};
