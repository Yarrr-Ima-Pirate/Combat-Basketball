// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
	
}

void UOverheadWidget::ShowPlayerName(APawn* InPawn)
{

	FString PlayerName;
	APlayerState* PlayerState = InPawn->GetPlayerState();
	if (PlayerState)
	{
		PlayerName = PlayerState->GetPlayerName();
	}
	FString RemoteRoleString = FString::Printf(TEXT("%s"), *PlayerName);
	SetDisplayText(RemoteRoleString);
	
	
	/**
	 *
	 *This code shows the network role of each pawn connected to the network. 
	ENetRole RemoteRole = InPawn->GetRemoteRole();
	FString Role;
	switch (RemoteRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy");
		break;
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;	
	}
	FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
	SetDisplayText(RemoteRoleString);
	*/
	
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
