// Fill out your copyright notice in the Description page of Project Settings.


#include "ArenaPlayerController.h"

#include "Components/ProgressBar.h"
#include "MenuSystem/HUD/ArenaHUD.h"
#include "MenuSystem/HUD/CharacterOverlay.h"
#include "Components/TextBlock.h"
#include "MenuSystem/GameMode/ArenaGameMode.h"
#include "MenuSystem/Character/ArenaBallCharacter.h"
#include "MenuSystem/HUD/Announcement.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "MenuSystem/ArenaComponents/CombatComponent.h"
#include "MenuSystem/GameState/ArenaGameState.h"
#include "MenuSystem/ArenaComponents/CombatComponent.h"

void AArenaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ArenaHUD = Cast<AArenaHUD>(GetHUD());
	ServerCheckMatchState();
}

void AArenaPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaPlayerController, MatchState);
}

void AArenaPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

void AArenaPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
    	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
    	{
    		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
    		TimeSyncRunningTime = 0.f;
    	}
}

void AArenaPlayerController::ServerCheckMatchState_Implementation()
{
	AArenaGameMode* GameMode = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void AArenaPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (ArenaHUD && MatchState == MatchState::WaitingToStart)
	{
		ArenaHUD->AddAnnouncement();
	}
}

void AArenaPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AArenaBallCharacter* ArenaBallCharacter = Cast<AArenaBallCharacter>(InPawn);
	if (ArenaBallCharacter)
	{
		SetHUDHealth(ArenaBallCharacter->GetHealth(), ArenaBallCharacter->GetMaxHealth());
	}
}

void AArenaPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	bool bHudValid = ArenaHUD &&
		ArenaHUD->CharacterOverlay &&
		ArenaHUD->CharacterOverlay->HealthBar &&
			ArenaHUD->CharacterOverlay->HealthText;
	if (bHudValid)
	{
		const float HealthPercent = Health / MaxHealth;
		ArenaHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"),FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		ArenaHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AArenaPlayerController::SetHUDScore(float SetScore)
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	bool bHudValid = ArenaHUD &&
		ArenaHUD->CharacterOverlay &&
		ArenaHUD->CharacterOverlay->ScoreAmount;
	if (bHudValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(SetScore));
		ArenaHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = SetScore;
	}
}

void AArenaPlayerController::SetHUDDefeats(int32 Defeats)
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	bool bHudValid = ArenaHUD &&
		ArenaHUD->CharacterOverlay &&
		ArenaHUD->CharacterOverlay->DefeatsAmount;
	if (bHudValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		ArenaHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void AArenaPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	bool bHudValid = ArenaHUD &&
		ArenaHUD->CharacterOverlay &&
		ArenaHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHudValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		ArenaHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}	
}

void AArenaPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	bool bHudValid = ArenaHUD &&
		ArenaHUD->CharacterOverlay &&
		ArenaHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHudValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		ArenaHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}		
}

void AArenaPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	bool bHudValid = ArenaHUD &&
		ArenaHUD->CharacterOverlay &&
		ArenaHUD->CharacterOverlay->MatchCountdownText;
	if (bHudValid)
	{
		if (CountdownTime < 0.f)
		{
			ArenaHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ArenaHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}	
}

void AArenaPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	bool bHudValid = ArenaHUD &&
		ArenaHUD->Announcement &&
		ArenaHUD->Announcement->WarmupTime;
	if (bHudValid)
	{
		if (CountdownTime < 0.f)
		{
			ArenaHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ArenaHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}		
}

void AArenaPlayerController::SetHUDHoldingBall(bool bIsHoldingBall)
{
	bIsHoldingBall = false;
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	bool bHudValid = ArenaHUD &&
		ArenaHUD->CharacterOverlay &&
		ArenaHUD->CharacterOverlay->HoldingBallImage &&
			bIsHoldingBall == false;
	if (bHudValid)
	{
		ArenaHUD->CharacterOverlay->HoldingBallImage->SetVisibility(ESlateVisibility::Hidden);
	}
	else if (ArenaHUD &&
		ArenaHUD->CharacterOverlay &&
		ArenaHUD->CharacterOverlay->HoldingBallImage && bIsHoldingBall == true)
	{
		ArenaHUD->CharacterOverlay->HoldingBallImage->SetVisibility(ESlateVisibility::Visible);
		bIsHoldingBall = true;
	}
	
}

void AArenaPlayerController::SetHUDTime()
{
	if (HasAuthority())
	{
		AArenaGameMode* BlasterGameMode = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this));
		if (BlasterGameMode)
		{
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
			//LevelStartingTime = BlasterGameMode->LevelStartingTime;
		}
	}
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		ArenaGameMode = ArenaGameMode == nullptr ? Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this)) : ArenaGameMode;
		if (ArenaGameMode)
		{
			SecondsLeft = FMath::CeilToInt(ArenaGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}
	
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void AArenaPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (ArenaHUD && ArenaHUD->CharacterOverlay)
		{
			CharacterOverlay = ArenaHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

void AArenaPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AArenaPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AArenaPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AArenaPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
	
}

void AArenaPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AArenaPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}	
}

void AArenaPlayerController::HandleMatchHasStarted()
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	if (ArenaHUD)
	{
		if (ArenaHUD->CharacterOverlay == nullptr) ArenaHUD->AddCharacterOverlay();
		if (ArenaHUD->Announcement)
		{
			ArenaHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}	
}

void AArenaPlayerController::HandleCooldown()
{
	ArenaHUD = ArenaHUD == nullptr ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD;
	if (ArenaHUD)
	{
		ArenaHUD->CharacterOverlay->RemoveFromParent();
		bool bHudValid = ArenaHUD->Announcement &&
			ArenaHUD->Announcement->AnnouncementText &&
				ArenaHUD->Announcement->InfoText;
		if (bHudValid)
		{
			ArenaHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			ArenaHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			AArenaGameState* ArenaGameState = Cast<AArenaGameState>(UGameplayStatics::GetGameState(this));
			AArenaPlayerState* ArenaPlayerState = GetPlayerState<AArenaPlayerState>();
			if (ArenaGameState && ArenaPlayerState)
			{
				TArray<AArenaPlayerState*> TopPlayers = ArenaGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == ArenaPlayerState)
				{
					InfoTextString = FString("You are the winner.");	
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win: \n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
				ArenaHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	AArenaBallCharacter* ArenaBallCharacter = Cast<AArenaBallCharacter>(GetPawn());
	if (ArenaBallCharacter && ArenaBallCharacter->GetCombat())
	{
		ArenaBallCharacter->bDisableGameplay = true;
		ArenaBallCharacter->GetCombat()->FireButtonPressed(false);
	}
}

