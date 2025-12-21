// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Shooter/ShooterGameState.h"
#include "Variant_Shooter/ShooterPlayerController.h"
#include "Net/UnrealNetwork.h"

AShooterGameState::AShooterGameState()
{
    TeamScores.SetNum(2);
}

void AShooterGameState::AddScore(uint8 Team, int32 Delta)
{
    if (HasAuthority() /* && TeamScores.IsValidIndex(Team)*/)
    {
        TeamScores[Team] += Delta;
        OnRep_TeamScores(); // 服务器自己也更新 UI（监听用）
    }
}

void AShooterGameState::OnRep_TeamScores()
{
    UE_LOG(LogTemp, Log, TEXT("Team0=%d Team1=%d"),
        TeamScores[0], TeamScores[1]);

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AShooterPlayerController* PC = Cast<AShooterPlayerController>(*It);
        if (PC && PC->IsLocalController())
        {
            PC->HandleTeamScoreUpdated(TeamScores);
        }
    }
}

void AShooterGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AShooterGameState, TeamScores);
}

