#pragma once

class UWorld;
class UObject;
class USoundBase;
class UParticleSystem;
class UParticleSystemComponent;
class UStaticMesh;
class UMaterialInterface;
class UAnimSequence;
class USkeletalMeshComponent;

/** Shared one-shot effects. Assets are rooted so gameplay code can reuse them. */
namespace SurvivorFeedback
{
	void PlayAt(const UObject* WorldContext, USoundBase* Sound, const FVector& Location, float Volume, float Pitch);
	UParticleSystemComponent* Burst(UWorld* World, const FVector& Location, UParticleSystem* System, float Scale);
	void SpawnShards(UWorld* World, const FVector& Origin, const FVector& Normal, int32 Count, UStaticMesh* Mesh, UMaterialInterface* Material, const FLinearColor& Tint, float Scale, float Speed);

	void PlayZombieHit(USkeletalMeshComponent* MeshComp, const FVector& FromLocation, bool bHeavy);
	void PlayZombieBite(USkeletalMeshComponent* MeshComp);

	UParticleSystem* Sparks();
	UParticleSystem* Smoke();
	UParticleSystem* Explosion();
	UParticleSystem* Dust();

	USoundBase* Collapse();
	USoundBase* Boom();
	USoundBase* SparkSound();
	USoundBase* Click();
	USoundBase* ClickSoft();
	USoundBase* Hiss();
	USoundBase* Wind();
	USoundBase* Music();
	USoundBase* Birds();

	UStaticMesh* Cube();
	UStaticMesh* Sphere();
}
