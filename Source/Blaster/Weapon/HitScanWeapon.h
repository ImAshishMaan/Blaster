#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon {
	GENERATED_BODY()
public:
	virtual void Fire(const FVector& HitTarget) override;

protected:

	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);
	
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticle;
	
	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;
	
	UPROPERTY(EditAnywhere)
	float Damage = 25.f;
	
private:


	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticle;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;

	

	
	
};
