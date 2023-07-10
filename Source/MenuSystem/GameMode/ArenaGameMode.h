// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MenuSystem/Character/ArenaBallCharacter.h"
#include "ArenaGameMode.generated.h"

namespace MatchState
{
	extern MENUSYSTEM_API const FName Cooldown; //Match duration has been reached. Display winner and cooldown timer.
}

/**
 * 
 */
UCLASS()
class MENUSYSTEM_API AArenaGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AArenaGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(AArenaBallCharacter* ElimmedCharacter, AArenaPlayerController* VictimController, AArenaPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;
	
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
	
	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
