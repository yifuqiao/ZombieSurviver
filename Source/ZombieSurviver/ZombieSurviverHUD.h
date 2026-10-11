// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once 
#include "GameFramework/HUD.h"
#include "ZombieSurviverHUD.generated.h"

UCLASS()
class AZombieSurviverHUD : public AHUD
{
	GENERATED_BODY()

public:
	AZombieSurviverHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	void DrawBar(float X, float Y, float Width, float Height, float Percent, const FLinearColor& Fill, const FString& Label);
	void DrawCrosshair(const FVector2D& Center, float Spread, const FLinearColor& Color);

	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};
