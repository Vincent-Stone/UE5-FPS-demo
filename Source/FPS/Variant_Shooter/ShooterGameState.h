// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ShooterGameState.generated.h"

/**
 * 
 */
UCLASS()
class FPS_API AShooterGameState : public AGameStateBase
{
	GENERATED_BODY()
	
	
public:
    AShooterGameState();

    UPROPERTY(ReplicatedUsing = OnRep_TeamScores)
    TArray<int32> TeamScores;

    UFUNCTION()
    void OnRep_TeamScores();

    void AddScore(uint8 Team, int32 Delta);

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps
    ) const override;
	
};
