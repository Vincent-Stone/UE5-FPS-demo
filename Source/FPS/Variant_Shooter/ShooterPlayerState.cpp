// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Shooter/ShooterPlayerState.h"
#include "Net/UnrealNetwork.h"

AShooterPlayerState::AShooterPlayerState()
{
	TeamByte = 254; // Œ¥∑÷≈‰
}

void AShooterPlayerState::OnRep_Team()
{
	// You can add any logic here that needs to happen when TeamByte is replicated
}

void AShooterPlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AShooterPlayerState, TeamByte);
}
