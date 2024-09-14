#include "BlasterGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* KillerController) {

	if(ElimmedCharacter) {
		ElimmedCharacter->Elim();
		UE_LOG(LogTemp, Warning, TEXT("ElimmedCharacter: %s"), *ElimmedCharacter->GetFullName());
	}
	
}
