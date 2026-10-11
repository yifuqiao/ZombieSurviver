#pragma once

#include "GameFramework/Actor.h"
#include "ImpactShard.generated.h"

/** Short-lived physics chip used for blood, splinters, and shell casings. */
UCLASS()
class ZOMBIESURVIVER_API AImpactShard : public AActor
{
	GENERATED_BODY()

public:
	AImpactShard();

	void InitializeShard(UStaticMesh* InMesh, UMaterialInterface* InMaterial, const FLinearColor& Tint, const FVector& Velocity, float InScale);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	class UStaticMeshComponent* Mesh;

	float UniformScale;
};
