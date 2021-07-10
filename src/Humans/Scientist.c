/****************************/
/*   	SCIENTIST.C		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/


static void MoveScientist(ObjNode *theNode);
static void MoveScientistOnSpline(ObjNode *theNode);
static void AbductScientist(ObjNode *theNode);
static void  MoveScientist_Standing(ObjNode *theNode);
static void  MoveScientist_Walking(ObjNode *theNode);
static void  MoveScientist_Teleport(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	SCIENTIST_SCALE			(2.0f * gHumanScaleRatio)


/*********************/
/*    VARIABLES      */
/*********************/

enum
{
	SCIENTIST_ANIM_STAND,
	SCIENTIST_ANIM_WALK,
	SCIENTIST_ANIM_ABDUCTED,
	SCIENTIST_ANIM_TELEPORT
};


#define	RingOffset	SpecialF[0]


/************************ ADD SCIENTIST *************************/

Boolean AddScientist(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

		/* MAKE OBJECT */

	newObj = MakeScientist(x,z);
	newObj->TerrainItemPtr = itemPtr;

		/* SEE IF ENCASED IN ICE */

	if (itemPtr->parm[3] & 1)
		EncaseHumanInIce(newObj);

	return(true);
}

/************************ MAKE SCIENTIST *************************/

ObjNode *MakeScientist(float x, float z)
{
ObjNode	*newObj;

		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_SCIENTIST;
	gNewObjectDefinition.animNum	= SCIENTIST_ANIM_STAND;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x,z,CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_SAUCERTARGET|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= HUMAN_SLOT;
	gNewObjectDefinition.moveCall	= MoveScientist;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= SCIENTIST_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->HumanType = HUMAN_TYPE_SCIENTIST;


				/* SET BETTER INFO */

	newObj->Coord.y 	-= newObj->BBox.min.y - 20.0f;
	UpdateObjectTransforms(newObj);

	newObj->Health 		= 1.0;

	newObj->SaucerTargetType = SAUCER_TARGET_TYPE_ABDUCT;
	newObj->SaucerAbductHandler = AbductScientist;


				/* SET COLLISION INFO */

	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_HUMAN;
	newObj->CType			= CTYPE_HUMAN|CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID|CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox(newObj, 1,1);



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8.0f * gHumanScaleRatio, 8.0f * gHumanScaleRatio, true);

	return(newObj);
}


/********************* MOVE SCIENTIST **************************/

static void MoveScientist(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveScientist_Standing,
					MoveScientist_Walking,
					MoveScientist_Standing,					// abducted
					MoveScientist_Teleport,
				};

	if (gLevelNum != LEVEL_NUM_SAUCER)
	{
		if (TrackTerrainItem(theNode))						// just check to see if it's gone
		{
			if (gSaucerTarget == theNode)					// see if was a saucer target
				gSaucerTarget = nil;
			DeleteObject(theNode);
			return;
		}
	}
	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE SCIENTIST: STANDING ******************************/

static void  MoveScientist_Standing(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	dx,dy,dz;

			/* MOVE */

	gDelta.x = gDelta.z = 0;					// no intertia while standing

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

	dx = gDelta.x;
	dy = gDelta.y;
	dz = gDelta.z;

	if (theNode->MPlatform)						// see if factor in moving platform
	{
		ObjNode *plat = theNode->MPlatform;
		dx += plat->Delta.x;
		dy += plat->Delta.y;
		dz += plat->Delta.z;
	}

	gCoord.x += dx*fps;
	gCoord.y += dy*fps;
	gCoord.z += dz*fps;


			/* COLLISION */

	DoHumanCollisionDetect(theNode);

	UpdateHuman(theNode);
}


/********************** MOVE SCIENTIST: WALKING ******************************/

static void  MoveScientist_Walking(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r;

	r = theNode->Rot.y;							// get aim
	gDelta.x = -sin(r) * 100.0f;				// set delta
	gDelta.z = -cos(r) * 100.0f;

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* COLLISION */

	DoHumanCollisionDetect(theNode);

	UpdateHuman(theNode);


			/* SEE IF DONE WALKING */

	theNode->HumanWalkTimer -= fps;
	if (theNode->HumanWalkTimer <= 0.0f)
		MorphToSkeletonAnim(theNode->Skeleton, 0, 3);				// go to stand anim
}

/********************** MOVE SCIENTIST: TELEPORT ******************************/

static void  MoveScientist_Teleport(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->ColorFilter.r = theNode->ColorFilter.b -= fps * .5f;

	UpdateObject(theNode);


			/* UPDATE TELEPORT EFFECT */

	if (UpdateHumanTeleport(theNode, true))				// returns true if human is deleted
		return;

}


#pragma mark -

/************************ PRIME SCIENTIST *************************/

Boolean PrimeScientist(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_SCIENTIST;
	gNewObjectDefinition.animNum	= SCIENTIST_ANIM_WALK;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_SAUCERTARGET|STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= HUMAN_SLOT;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= SCIENTIST_SCALE;
	gNewObjectDefinition.moveCall 	= MoveScientist;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);


	newObj->Skeleton->AnimSpeed = 1.5;

	newObj->HumanType = HUMAN_TYPE_SCIENTIST;


				/* SET MORE INFO */

	newObj->SaucerTargetType = SAUCER_TARGET_TYPE_ABDUCT;
	newObj->SaucerAbductHandler = AbductScientist;

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveScientistOnSpline;						// set move call
	newObj->CBits			= CBITS_ALLSOLID|CBITS_ALWAYSTRIGGER;

	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_HUMAN;
	newObj->CType			= CTYPE_MISC|CTYPE_HUMAN|CTYPE_TRIGGER;

	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8.0f * gHumanScaleRatio, 8.0f * gHumanScaleRatio, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);									// detach this object from the linked list
	AddToSplineObjectList(newObj, true);


	return(true);
}


/******************** MOVE SCIENTIST ON SPLINE ***************************/

static void MoveScientistOnSpline(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 30.0f * gHumanScaleRatio);

	GetObjectCoordOnSpline(theNode);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box

		UpdateHuman(theNode);
		UpdateShadow(theNode);
	}

			/* NOT VISIBLE */
	else
	{
	}
}

#pragma mark -

/**************** ABDUCT SCIENTIST **************************/
//
// Callback called when alien saucer is in position and ready to abduct.
//

static void AbductScientist(ObjNode *theNode)
{
			/* SEE IF REMOVE FROM SPLINE */

	if (theNode->StatusBits & STATUS_BIT_ONSPLINE)
		DetachObjectFromSpline(theNode, nil);

	theNode->CType = 0;					// no longer interesting
	theNode->Delta.x =
	theNode->Delta.y =
	theNode->Delta.z = 0;				// stop him

	MorphToSkeletonAnim(theNode->Skeleton, SCIENTIST_ANIM_ABDUCTED, 6);

	theNode->TerrainItemPtr = nil;		// dont come back

	if (gLevelNum == LEVEL_NUM_SAUCER)	// if abducted by player saucer then change move call
		theNode->MoveCall = MoveHuman_ToPlayerSaucer;
	else
		theNode->MoveCall = nil;

}










