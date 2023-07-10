// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MenuSystem/PlayerState/ArenaPlayerState.h"
#include "ArenaGameState.generated.h"

/**
 * 
 */
UCLASS()
class MENUSYSTEM_API AArenaGameState : public AGameState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(AArenaPlayerState* ScoringPlayer);
	
	UPROPERTY(Replicated)
	TArray<AArenaPlayerState*> TopScoringPlayers;
private:

	float TopScore = 0.f;
};
