// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/HUD.h"
#include "Crosshair.generated.h"

/**
 * 
 */
UCLASS()
class GRAPPLESTUDY_API ACrosshair : public AHUD
{
	GENERATED_BODY()
	
public:
	ACrosshair();
	void DrawHUD();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		UTexture* Crosshair;

	UFUNCTION()
		FVector2D GetGameViewportSize();

	UFUNCTION()
		FVector2D GetGameResolution();
	
};
