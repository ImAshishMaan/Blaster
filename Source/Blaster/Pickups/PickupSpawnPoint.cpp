#include "PickupSpawnPoint.h"
#include "Pickup.h"
#include "TimerManager.h"
#include "Engine/World.h"

APickupSpawnPoint::APickupSpawnPoint() {
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay() {
	Super::BeginPlay();

	StartPickupTimer((AActor*)nullptr);
}
void APickupSpawnPoint::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void APickupSpawnPoint::SpawnPickup() {
	int32 NumPickupClasses = PickUpClasses.Num();
	if(NumPickupClasses > 0) {
		const int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickUpClasses[Selection], GetActorTransform());
		if(HasAuthority() && SpawnedPickup) {
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartPickupTimer);
		}
	}
}

void APickupSpawnPoint::StartPickupTimer(AActor* DestroyedActor) {
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	GetWorldTimerManager().SetTimer(SpawnPickupTimerHandle, this, &APickupSpawnPoint::SpawnPickupTimerFinished, SpawnTime);
}

void APickupSpawnPoint::SpawnPickupTimerFinished() {
	if(HasAuthority()) {
		SpawnPickup();
	}
}
