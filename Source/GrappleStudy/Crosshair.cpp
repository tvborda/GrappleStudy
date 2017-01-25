// Fill out your copyright notice in the Description page of Project Settings.

#include "GrappleStudy.h"
#include "Crosshair.h"

ACrosshair::ACrosshair()
{
	Crosshair = ConstructorHelpers::FObjectFinderOptional<UTexture>(TEXT("Texture2D'/Game/Textures/FirstPersonCrosshair.FirstPersonCrosshair'")).Get();
}

void ACrosshair::DrawHUD() {
	const FVector2D ViewPortSize = GetGameViewportSize();
	DrawTexture(Crosshair, (ViewPortSize.X / 2) - 8.0f, (ViewPortSize.Y / 2) - 8.0f, 16.0f, 16.0f, 0.0f, 0.0f, 1.0f, 1.0f);
}

FVector2D ACrosshair::GetGameViewportSize()
{
	FVector2D Result = FVector2D(1, 1);

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize( /*out*/Result);
	}

	return Result;
}

FVector2D ACrosshair::GetGameResolution()
{
	FVector2D Result = FVector2D(1, 1);

	Result.X = GSystemResolution.ResX;
	Result.Y = GSystemResolution.ResY;

	return Result;
}
