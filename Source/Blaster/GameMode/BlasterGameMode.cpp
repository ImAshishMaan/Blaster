#include "BlasterGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* KillerController) {

	if(ElimmedCharacter) {
		ElimmedCharacter->Elim();
		UE_LOG(LogTemp, Warning, TEXT("ElimmedCharacter: %s"), *ElimmedCharacter->GetFullName());
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
