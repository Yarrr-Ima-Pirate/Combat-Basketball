// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "BasketballProjectile.generated.h"

/**
 * 
 */
UCLASS()
class MENUSYSTEM_API ABasketballProjectile : public AProjectile
{
	GENERATED_BODY()
public:
	ABasketballProjectile();

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;
	
	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;
};
