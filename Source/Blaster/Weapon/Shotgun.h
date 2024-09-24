#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon {
	GENERATED_BODY()
public:

	virtual void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);
	void ShotgunTraceEndWithScatter(const FVector_NetQuantize& HitTarget, TArray<FVector_NetQuantize>& HitTargets);

private:

	UPROPERTY(EditAnywhere)
	uint32 NumberOfPellets = 10;
	
};
