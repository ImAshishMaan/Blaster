#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode() {
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay() {
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	if(MatchState == MatchState::WaitingToStart) {
		CountdownTime += DeltaSeconds;
		if(CountdownTime >= WarmupTime) {
			StartMatch();
		}
	}else if(MatchState	== MatchState::InProgress) {
		//CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		CountdownTime += DeltaSeconds;
		if(CountdownTime >= WarmupTime + MatchTime) {
			SetMatchState(MatchState::Cooldown);
		}
	}else if(MatchState == MatchState::Cooldown) {
		CountdownTime += DeltaSeconds;
		if(CountdownTime >= CooldownTime) {
			RestartGame();
		}
	}
}

void ABlasterGameMode::OnMatchStateSet() {
	Super::OnMatchStateSet();

	FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator();
	while(Iterator) {
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(Iterator->Get());
		if(BlasterPlayer) {
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
		++Iterator;
	}
	
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController) {

	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	
	if(AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState) {
		AttackerPlayerState->AddToScore(15.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);
	}
	if(VictimPlayerState) {
		VictimPlayerState->AddToDefeats(1);
	}
	if(ElimmedCharacter) {
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController) {

	if(ElimmedController) {
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}

	if(ElimmedController) {
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
	
}

