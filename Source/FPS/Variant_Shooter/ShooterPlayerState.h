// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShooterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class FPS_API AShooterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	AShooterPlayerState();

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	uint8 TeamByte;

	UFUNCTION()
	void OnRep_Team();

	uint8 GetTeam() const { return TeamByte; }
};
