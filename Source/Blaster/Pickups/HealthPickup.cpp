#include "HealthPickup.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AHealthPickup::AHealthPickup() {
	bReplicates = true;

	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>("PickupEffectComponent");
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                    const FHitResult& SweepResult) {
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if(BlasterCharacter) {
		UBuffComponent* Buff = BlasterCharacter->GetBuff();
		if(Buff) {
			Buff->Heal(HealAmount, HealingTime);
			UE_LOG(LogTemp, Warning, TEXT("Buff->Heal"));
		}
	}

	Destroy();
}

void AHealthPickup::Destroyed() {
	if(PickupEffect) {
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation()
		);
	}
	Super::Destroyed();
}
