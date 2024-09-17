#pragma once

#include "CoreMinimal.h"
#include "AnnouncementWidget.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UCharacterOverlayWidget;
class UTexture2D;

USTRUCT(BlueprintType)
struct FHUDPackage {
	GENERATED_BODY()

public:
	UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;
	
	float CrosshairsSpread;

	FLinearColor CrosshairsColor;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD {
	GENERATED_BODY()

public:

	virtual void DrawHUD() override;

	void InitOverlays(); // Initialize all overlays on BeginPlay

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	UCharacterOverlayWidget* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> AnnouncementOverlayClass;

	UPROPERTY()
	UAnnouncementWidget* AnnouncementOverlay;
	
	/*
	 * Add Character Overlay to the viewport
	 */
	void AddCharacterOverlay();
	void AddAnnouncementOverlay();

protected:

	virtual void BeginPlay() override;

private:

	FHUDPackage HUDPackage;

	void DrawCrosshairs(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairsColor);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	
};
