// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"
//#include "ShooterUI.h"
#include "Variant_Shooter/ShooterPlayerState.h"
#include "Variant_Shooter/ShooterGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

uint8 AShooterGameMode::NextTeam;

AShooterGameMode::AShooterGameMode()
{
	// set default classes
	//HUDClass = AShooterHUD::StaticClass();
	//PlayerControllerClass = AShooterPlayerController::StaticClass();
	//DefaultPawnClass = AFPSCharacter::StaticClass();
	PlayerStateClass = AShooterPlayerState::StaticClass();
	NextTeam = 1;
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// create the UI
	//ShooterUI = CreateWidget<UShooterUI>(UGameplayStatics::GetPlayerController(GetWorld(), 0), ShooterUIClass);
	//ShooterUI->AddToViewport(0);
}

void AShooterGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AShooterPlayerState* PS =
		Cast<AShooterPlayerState>(NewPlayer->PlayerState);

	if (!PS) return;

	// 示例：简单轮流分队
	PS->TeamByte = GetNextTeam();
}

uint8 AShooterGameMode::GetNextTeam()
{
	uint8 Assigned = NextTeam;
	NextTeam = (NextTeam + 1) % 2; // 两队
	return Assigned;
}


void AShooterGameMode::IncrementTeamScore(uint8 TeamByte)
{
	AShooterGameState* GS = GetGameState<AShooterGameState>();
	if (GS)
	{
		GS->AddScore(TeamByte, 1);
	}


	/*// retrieve the team score if any
	int32 Score = 0;
	if (int32* FoundScore = TeamScores.Find(TeamByte))
	{
		Score = *FoundScore;
	}

	// increment the score for the given team
	++Score;
	TeamScores.Add(TeamByte, Score);
	for (auto it = TeamScores.begin(); it != TeamScores.end(); ++it) {
		UE_LOG(LogTemp, Warning, TEXT("Team:%d	Score:%d"),it->Key,it->Value);
	}
	// update the UI
	//ShooterUI->BP_UpdateScore(TeamByte, Score);*/
}
