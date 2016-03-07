

#pragma once

#include "AIController.h"
#include "AIBrain.generated.h"

/**
 * 
 */

UCLASS()
class ZOMBIESURVIVER_API AAIBrain : public AAIController
{
	GENERATED_BODY()
	
public:
	AAIBrain(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;

	void SetTarget(AActor* player);

private:
	
	AActor* m_targetPlayer;
};
