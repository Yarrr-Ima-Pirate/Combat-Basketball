// Fill out your copyright notice in the Description page of Project Settings.


#include "ArenaGameMode.h"

#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "MenuSystem/PlayerController/ArenaPlayerController.h"
#include "MenuSystem/Character/ArenaBallCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerStart.h"
#include "MenuSystem/PlayerState/ArenaPlayerState.h"
#include "MenuSystem//GameState/ArenaGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AArenaGameMode::AArenaGameMode()
{
	bDelayedStart = true;
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AArenaGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void AArenaGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AArenaPlayerController* ArenaPlayer = Cast<AArenaPlayerController>(*It);
		if (ArenaPlayer)
		{
			ArenaPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void AArenaGameMode::PlayerEliminated(AArenaBallCharacter* ElimmedCharacter, AArenaPlayerController* VictimController,
                                      AArenaPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	AArenaPlayerState* AttackerPlayerState = AttackerController ? Cast<AArenaPlayerState>(AttackerController->PlayerState) : nullptr;
	AArenaPlayerState* VictimPlayerState = VictimController ? Cast<AArenaPlayerState>(VictimController->PlayerState) : nullptr;

	AArenaGameState* ArenaGameState = GetGameState<AArenaGameState>();
	
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && ArenaGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		ArenaGameState->UpdateTopScore(AttackerPlayerState);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
		
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void AArenaGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() -1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
