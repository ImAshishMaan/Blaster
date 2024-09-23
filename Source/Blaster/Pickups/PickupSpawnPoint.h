#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor {
	GENERATED_BODY()

public:
	APickupSpawnPoint();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<APickup>> PickUpClasses;

	UPROPERTY()
	APickup* SpawnedPickup;

	void SpawnPickup();
	void SpawnPickupTimerFinished();
	UFUNCTION()
	void StartPickupTimer(AActor* DestroyedActor);
private:
	FTimerHandle SpawnPickupTimerHandle;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;
	
};
