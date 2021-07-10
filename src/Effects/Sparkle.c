/****************************/
/*   	SPARKLE.C  			*/
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



/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

SparkleType	gSparkles[MAX_SPARKLES];

static float	gPlayerSparkleColor = 0;

int	gNumSparkles;

/*************************** INIT SPARKLES **********************************/

void InitSparkles(void)
{
int		i;

	for (i = 0; i < MAX_SPARKLES; i++)
	{
		gSparkles[i].isActive = false;
	}

	gPlayerSparkleColor = 0;

	gNumSparkles = 0;
}


/*************************** GET FREE SPARKLE ******************************/
//
// OUTPUT:  -1 if none
//

short GetFreeSparkle(ObjNode *theNode)
{
int		i;

			/* FIND A FREE SLOT */

	for (i = 0; i < MAX_SPARKLES; i++)
	{
		if (!gSparkles[i].isActive)
			goto got_it;

	}
	return(-1);

got_it:

	gSparkles[i].isActive = true;
	gSparkles[i].owner = theNode;
	gNumSparkles++;

	return(i);
}




/***************** DELETE SPARKLE *********************/

void DeleteSparkle(short i)
{
	if (i == -1)
		return;

	if (gSparkles[i].isActive)
	{
		gSparkles[i].isActive = false;
		gNumSparkles--;
	}
//	else
//		DoAlert("DeleteSparkle: double delete sparkle");
}


/*************************** DRAW SPARKLES ******************************/

void DrawSparkles(OGLSetupOutputType *setupInfo)
{
u_long	flags;
int		i;
float	dot,separation;
OGLMatrix4x4	m;
OGLVector3D	v;
OGLPoint3D	where;
static const OGLVector3D 	up = {0,1,0};
OGLPoint3D					tc[4], *cameraLocation;
static OGLPoint3D		frame[4] =
{
	{-130,	130,	0},
	{130,	130,	0},
	{130,	-130,	0},
	{-130,	-130,	0},
};


	OGL_PushState();

	OGL_DisableLighting();									// deactivate lighting
	glDisable(GL_NORMALIZE);								// disable vector normalizing since scale == 1
	glDisable(GL_CULL_FACE);								// deactivate culling
	glDisable(GL_FOG);										// deactivate fog
	glDepthMask(GL_FALSE);									// no z-writes
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);		// set blending mode


			/*********************/
			/* DRAW EACH SPARKLE */
			/*********************/

	cameraLocation = &setupInfo->cameraPlacement.cameraLocation;		// point to camera coord

	for (i = 0; i < MAX_SPARKLES; i++)
	{
		ObjNode	*owner;

		if (!gSparkles[i].isActive)							// must be active
			continue;

		flags = gSparkles[i].flags;							// get sparkle flags

		owner = gSparkles[i].owner;
		if (owner != nil)									// if owner is culled then dont draw
		{
			if (owner->StatusBits & (STATUS_BIT_ISCULLED|STATUS_BIT_HIDDEN))
				continue;

					/* SEE IF TRANSFORM WITH OWNER */

			if (flags & SPARKLE_FLAG_TRANSFORMWITHOWNER)
			{
				OGLPoint3D_Transform(&gSparkles[i].where, &owner->BaseTransformMatrix, &where);
			}
			else
				where = gSparkles[i].where;
		}
		else
			where = gSparkles[i].where;


			/* CALC ANGLE INFO */

		v.x = cameraLocation->x - where.x;			// calc vector from sparkle to camera
		v.y = cameraLocation->y - where.y;
		v.z = cameraLocation->z - where.z;
		FastNormalizeVector(v.x, v.y, v.z, &v);

		separation = gSparkles[i].separation;
		where.x += v.x * separation;									// offset the base point
		where.y += v.y * separation;
		where.z += v.z * separation;

		if (!(flags & SPARKLE_FLAG_OMNIDIRECTIONAL))		// if not omni then calc alpha based on angle
		{
			dot = OGLVector3D_Dot(&v, &gSparkles[i].aim);				// calc angle between
			if (dot <= 0.0f)
				continue;

			gSparkles[i].color.a = dot;									// make brighter as we look head-on
		}



			/* CALC TRANSFORM MATRIX */

		frame[0].x = -gSparkles[i].scale;								// set size of quad
		frame[0].y = gSparkles[i].scale;
		frame[1].x = gSparkles[i].scale;
		frame[1].y = gSparkles[i].scale;
		frame[2].x = gSparkles[i].scale;
		frame[2].y = -gSparkles[i].scale;
		frame[3].x = -gSparkles[i].scale;
		frame[3].y = -gSparkles[i].scale;

		SetLookAtMatrixAndTranslate(&m, &up, &where, cameraLocation);	// aim at camera & translate
		OGLPoint3D_TransformArray(&frame[0], &m, tc, 4);



			/* DRAW IT */

		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][gSparkles[i].textureNum].materialObject, setupInfo);	// submit material

		if (flags & SPARKLE_FLAG_FLICKER)								// set transparency
		{
			OGLColorRGBA	c = gSparkles[i].color;

			c.a += RandomFloat2() * .5f;
			if (c.a < 0.0)
				continue;
			else
			if (c.a > 1.0f)
				c.a = 1.0;

			SetColor4fv((GLfloat *)&c);
		}
		else
			SetColor4fv((GLfloat *)&gSparkles[i].color);


		glBegin(GL_QUADS);
		glTexCoord2f(0,0);	glVertex3fv((GLfloat *)&tc[0]);
		glTexCoord2f(1,0);	glVertex3fv((GLfloat *)&tc[1]);
		glTexCoord2f(1,1);	glVertex3fv((GLfloat *)&tc[2]);
		glTexCoord2f(0,1);	glVertex3fv((GLfloat *)&tc[3]);
		glEnd();
	}


			/* RESTORE STATE */

	OGL_PopState();
}


#pragma mark -


/*************** CREATE PLAYER SPARKLES ********************/

void CreatePlayerSparkles(ObjNode *theNode)
{
short	i;

			/* CREATE CHEST LIGHTS */

	i = theNode->Sparkles[PLAYER_SPARKLE_CHEST] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i == -1)
		return;

	gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
	gSparkles[i].where.x = theNode->Coord.x;
	gSparkles[i].where.y = theNode->Coord.y + 100.0f;
	gSparkles[i].where.z = theNode->Coord.z;

	gSparkles[i].aim.x =
	gSparkles[i].aim.y =
	gSparkles[i].aim.z = 1;

	gSparkles[i].color.r = 1;
	gSparkles[i].color.g = 1;
	gSparkles[i].color.b = .3;
	gSparkles[i].color.a = .7;

	gSparkles[i].scale = 80.0f;
	gSparkles[i].separation = 30.0f;

	gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
}

/***************** CREATE PLAYER JUMPJET SPARKLES **********************/

void CreatePlayerJumpJetSparkles(ObjNode *theNode)
{
short	i;

			/* LEFT FOOT */

	i = theNode->Sparkles[PLAYER_SPARKLE_LEFTFOOT] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i == -1)
		return;

	gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
	gSparkles[i].where.x = theNode->Coord.x;
	gSparkles[i].where.y = theNode->Coord.y;;
	gSparkles[i].where.z = theNode->Coord.z;

	gSparkles[i].aim.x =
	gSparkles[i].aim.y =
	gSparkles[i].aim.z = 1;

	gSparkles[i].color.r = 1;
	gSparkles[i].color.g = 1;
	gSparkles[i].color.b = 1;
	gSparkles[i].color.a = 1;

	gSparkles[i].scale = 300.0f  * gPlayerInfo.scaleRatio;
	gSparkles[i].separation = 10.0f;

	gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark3;


			/* RIGHT FOOT */

	i = theNode->Sparkles[PLAYER_SPARKLE_RIGHTFOOT] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i == -1)
		return;

	gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
	gSparkles[i].where.x = theNode->Coord.x;
	gSparkles[i].where.y = theNode->Coord.y;;
	gSparkles[i].where.z = theNode->Coord.z;

	gSparkles[i].aim.x =
	gSparkles[i].aim.y =
	gSparkles[i].aim.z = 1;

	gSparkles[i].color.r = 1;
	gSparkles[i].color.g = 1;
	gSparkles[i].color.b = 1;
	gSparkles[i].color.a = 1;

	gSparkles[i].scale = 300.0f  * gPlayerInfo.scaleRatio;
	gSparkles[i].separation = 10.0f;

	gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark3;

}



/*************** UPDATE PLAYER SPARKLES ********************/

void UpdatePlayerSparkles(ObjNode *theNode)
{
short			i;
OGLMatrix4x4	m;
float			fps = gFramesPerSecondFrac;

		/*****************************/
		/* UPDATE THE CHEST SPARKLES */
		/*****************************/

	i = theNode->Sparkles[PLAYER_SPARKLE_CHEST];								// get sparkle index
	if (i != -1)
	{
		static const OGLPoint3D	p = {0,0,0};

				/* GET MATRIX FOR JOINT */

		FindJointFullMatrix(theNode, PLAYER_JOINT_TORSO, &m);


				/* CALC COORD */

		OGLPoint3D_Transform(&p, &m, &gSparkles[i].where);


		gPlayerSparkleColor += fps * 10.0f;
		gSparkles[i].color.r = (1.0f + sin(gPlayerSparkleColor)) * .5f;
		gSparkles[i].scale = 80.0f * gPlayerInfo.scaleRatio;
	}


			/***************************/
			/* UPDATE JUMP-JET SPARKLE */
			/***************************/

				/* LEFT FOOT */

	i = theNode->Sparkles[PLAYER_SPARKLE_LEFTFOOT];								// get sparkle index
	if (i != -1)
	{
		static const OGLPoint3D	p = {0,-30,-20};


		FindJointFullMatrix(theNode, PLAYER_JOINT_LEFTFOOT, &m);
		OGLPoint3D_Transform(&p, &m, &gSparkles[i].where);

		gSparkles[i].color.a -= fps * .6f;						// fade out
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[PLAYER_SPARKLE_LEFTFOOT] = -1;
		}
	}


				/* RIGHT FOOT */

	i = theNode->Sparkles[PLAYER_SPARKLE_RIGHTFOOT];								// get sparkle index
	if (i != -1)
	{
		static const OGLPoint3D	p = {0,-30,-20};

		FindJointFullMatrix(theNode, PLAYER_JOINT_RIGHTFOOT, &m);
		OGLPoint3D_Transform(&p, &m, &gSparkles[i].where);

		gSparkles[i].color.a -= fps * .6f;						// fade out
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[PLAYER_SPARKLE_RIGHTFOOT] = -1;
		}
	}


			/*************************/
			/* UPDATE MUZZLE SPARKLE */
			/*************************/

	i = theNode->Sparkles[PLAYER_SPARKLE_MUZZLEFLASH];								// get sparkle index
	if (i != -1)
	{
		FindCoordOnJoint(theNode, PLAYER_JOINT_LEFTHAND, &gPlayerMuzzleTipOff, &gSparkles[i].where);

		gSparkles[i].color.a -= fps * 4.0f;						// fade out
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[PLAYER_SPARKLE_MUZZLEFLASH] = -1;
		}
	}


}


#pragma mark -


/***************** ADD RUNWAY LIGHTS ***********************/

Boolean AddRunwayLights(TerrainItemEntryType *itemPtr, long  x, long z)
{
u_char	flicker = itemPtr->parm[3] & 1;						// see if flicker
ObjNode	*newObj;
short	i,numSparkles,j,t;
float	r,dx,dz,x2,z2;
static const short textureTable[] =
{
	PARTICLE_SObjType_WhiteSpark,
	PARTICLE_SObjType_WhiteSpark2,
	PARTICLE_SObjType_WhiteSpark3,
	PARTICLE_SObjType_WhiteSpark4,
	PARTICLE_SObjType_RedGlint,
	PARTICLE_SObjType_RedSpark,
	PARTICLE_SObjType_BlueSpark,
	PARTICLE_SObjType_GreenSpark,
	PARTICLE_SObjType_BlueGlint
};

	t = itemPtr->parm[2];									// get texture #
	t = textureTable[t];

	numSparkles = itemPtr->parm[1];
	if (numSparkles > MAX_NODE_SPARKLES)
		DoFatalAlert("AddRunwayLights: # lights > MAX_NODE_SPARKLES");
	if (numSparkles == 0)
		numSparkles = 1;

			/*********************/
			/* MAKE EVENT OBJECT */
			/*********************/

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+20;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/*********************/
			/* MAKE THE SPARKLES */
			/*********************/

	r = (float)itemPtr->parm[0] * (PI/8);

	dx = -sin(r) * (TERRAIN_POLYGON_SIZE/2);
	dz = -cos(r) * (TERRAIN_POLYGON_SIZE/2);
	x2 = x;
	z2 = z;

	for (j = 0; j < numSparkles; j++)
	{
		i = newObj->Sparkles[j] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (i != -1)
		{
			if (flicker)
				gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_FLICKER;
			else
				gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;

			gSparkles[i].where.x = x2;
			gSparkles[i].where.z = z2;
			gSparkles[i].where.y = GetTerrainY(x2,z2) + 20.0f;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 60.0f;

			gSparkles[i].separation = 40.0f;

			gSparkles[i].textureNum = t;
		}


		x2 += dx;
		z2 += dz;
	}



	return(true);													// item was added
}

















