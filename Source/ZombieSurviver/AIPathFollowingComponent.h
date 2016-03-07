

#pragma once

#include "Navigation/PathFollowingComponent.h"
#include "AIPathFollowingComponent.generated.h"

/**
 * 
 */
class ARecastNavMesh;
UCLASS()
class ZOMBIESURVIVER_API UAIPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()
	
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//	  Per Tick Modification of Path Following
		//			this is how you really customize 
		//				how units follow the path! 

	/** follow current path segment */
	virtual void FollowPathSegment(float DeltaTime) override;

	/** sets variables related to current move segment */
	virtual void SetMoveSegment(int32 SegmentStartIndex) override;

	/** check state of path following, update move segment if needed */
	virtual void UpdatePathSegment() override;
	
	//add this to your custom path follow component!
	bool NavPoly_GetAllPolys(TArray<NavNodeRef>& Polys);

	//Nav Data Main
	ANavigationData* GetMainNavData(FNavigationSystem::ECreateIfEmpty CreateNewIfNoneFound);

	//Choose Which Nav Data To Use
	ANavigationData* JoyGetNavData();

	//VERY IMPORTANT FOR CRASH PROTECTION !!!!!
	bool TileIsValid(const ARecastNavMesh* NavMesh, int32 TileIndex);

};
