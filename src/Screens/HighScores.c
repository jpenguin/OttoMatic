/****************************/
/*   	HIGHSCORES.C    	*/
/* (c)1996-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupScoreScreen(void);
static void FreeScoreScreen(void);
static void DrawHighScoresCallback(OGLSetupOutputType *info);
static void DrawScoreVerbage(OGLSetupOutputType *info);
static void DrawHighScoresAndCursor(OGLSetupOutputType *info);
static void SetHighScoresSpriteState(void);
static void StartEnterName(void);
static void MoveHighScoresCyc(ObjNode *theNode);
static void MoveScoreVerbageTally(ObjNode* theNode);
static Boolean IsThisScoreInList(uint32_t score);
static short AddNewScore(uint32_t newScore);
static void SaveHighScores(void);

/***************************/
/*    CONSTANTS            */
/***************************/

enum
{
	HIGHSCORES_SObjType_Cyc
};


enum
{
	HIGHSCORES_SObjType_ScoreText,
	HIGHSCORES_SObjType_ScoreTextGlow,
	HIGHSCORES_SObjType_EnterNameText,
	HIGHSCORES_SObjType_EnterNameGlow
};


#define kHSTableYStart		(-140)
#define kHSTableScale		.7f
#define kHSTableXSpread		150
#define kHSTableLineHeight	20


/***************************/
/*    VARIABLES            */
/***************************/

static const char*	gHighScoresFileName = ":OttoMatic:HighScores2";

HighScoreType	gHighScores[NUM_SCORES];

static	float	gFinalScoreTimer,gFinalScoreAlpha, gCursorFlux = 0;

static	short	gNewScoreSlot,gCursorIndex;

static	Boolean	gDrawScoreVerbage,gExitHighScores;

static	ObjNode	*gNewScoreNameMesh = nil;
static	ObjNode	*gCursorMesh = nil;


/*********************** NEW SCORE ***********************************/

static void UpdateNewNameMesh(void)
{
	GAME_ASSERT(gNewScoreNameMesh);
	DeleteObject(gNewScoreNameMesh);

	memset(&gNewObjectDefinition, 0, sizeof(gNewObjectDefinition));
	gNewObjectDefinition.slot		= SLOT_OF_DUMB+100;
	gNewObjectDefinition.moveCall	= nil;
	gNewObjectDefinition.flags		= 0;
	gNewObjectDefinition.scale		= kHSTableScale;
	gNewObjectDefinition.coord.x 	= -kHSTableXSpread;
	gNewObjectDefinition.coord.y 	= kHSTableYStart + kHSTableLineHeight * gNewScoreSlot;
	gNewObjectDefinition.coord.z 	= 0;

	gNewScoreNameMesh = TextMesh_New(gHighScores[gNewScoreSlot].name, 0, &gNewObjectDefinition);
}

static void UpdateCursorPos(void)
{
	GAME_ASSERT(gCursorMesh);

	gCursorMesh->Coord.x = -kHSTableXSpread + kHSTableScale * .5f * TextMesh_GetCharX(gHighScores[gNewScoreSlot].name, gCursorIndex) - 2.5;
	UpdateObjectTransforms(gCursorMesh);
}

void NewScore(void)
{
	if (gScore == 0)
		return;

	gAllowAudioKeys = false;					// dont interfere with name editing

			/* INIT */


	LoadHighScores();										// make sure current scores are loaded
	SetupScoreScreen();										// setup OGL
	MakeFadeEvent(true, 1.0);


			/* LOOP */

	CalcFramesPerSecond();
	UpdateInput();

	char* myName = gHighScores[gNewScoreSlot].name;

	while(!gExitHighScores)
	{

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawHighScoresCallback);

				/*****************************/
				/* SEE IF USER ENTERING NAME */
				/*****************************/

		if (!gDrawScoreVerbage)
		{
			if (GetNewKeyState(SDL_SCANCODE_RETURN) || GetNewKeyState(SDL_SCANCODE_KP_ENTER))
			{
				gExitHighScores = true;
			}
			else if (GetNewKeyState(SDL_SCANCODE_LEFT))
			{
				if (gCursorIndex > 0)
					gCursorIndex--;

				UpdateCursorPos();
			}
			else if (GetNewKeyState(SDL_SCANCODE_RIGHT))
			{
				short myLength = strlen(myName);
				if (gCursorIndex < myLength)
					gCursorIndex++;
				else
					gCursorIndex = myLength;

				UpdateCursorPos();
			}
			else if (GetNewKeyState(SDL_SCANCODE_HOME))
			{
				gCursorIndex = 0;
				UpdateCursorPos();
			}
			else if (GetNewKeyState(SDL_SCANCODE_END))
			{
				gCursorIndex = strlen(myName);
				UpdateCursorPos();
			}
			else if (GetNewKeyState(SDL_SCANCODE_BACKSPACE))
			{
				if (gCursorIndex > 0)
				{
					gCursorIndex--;

					for (int i = gCursorIndex; i < MAX_NAME_LENGTH; i++)
						myName[i] = myName[i+1];

					UpdateNewNameMesh();
					UpdateCursorPos();
				}
			}
			else if (GetNewKeyState(SDL_SCANCODE_DELETE))
			{
				if (gCursorIndex < MAX_NAME_LENGTH)
				{
					for (int i = gCursorIndex; i < MAX_NAME_LENGTH; i++)
						myName[i] = myName[i+1];
					UpdateNewNameMesh();
					UpdateCursorPos();
				}
			}
			else if (gTextInput[0] && gTextInput[0] >= ' ' && gTextInput[0] < '~')
			{
				// The text event gives us UTF-8. Use the key only if it's a printable ASCII character.
				char theChar = gTextInput[0];
				if (gCursorIndex < MAX_NAME_LENGTH)								// dont add anything more if maxxed out now
				{
					if ((theChar >= 'a') && (theChar <= 'z'))					// see if convert lower case to upper case a..z
						theChar = 'A' + (theChar-'a');

					for (int i = MAX_NAME_LENGTH-1; i > gCursorIndex; i--)
						myName[i] = myName[i-1];

					myName[gCursorIndex] = theChar;
					gCursorIndex++;

					UpdateNewNameMesh();
					UpdateCursorPos();
				}
			}

			GAME_ASSERT(myName[MAX_NAME_LENGTH] == '\0');
		}
	}


		/* CLEANUP */

	if (gNewScoreSlot != -1)						// if a new score was added then update the high scores file
		SaveHighScores();

	GammaFadeOut();

	FreeScoreScreen();


	gAllowAudioKeys = true;
}



ObjNode* SetupHighScoreTableObjNodes(ObjNode* chainTail, int returnNth)
{
	ObjNode* toReturn = nil;

		/* MAKE TEXT NODES */

	gNewObjectDefinition.slot		= SLOT_OF_DUMB+100;
	gNewObjectDefinition.moveCall	= nil;
	gNewObjectDefinition.flags		= 0;
	gNewObjectDefinition.scale		= kHSTableScale;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= kHSTableYStart;
	gNewObjectDefinition.coord.z 	= 0;

	for (int i = 0; i < NUM_SCORES; i++)
	{
		Str255 scratch;

			/* DRAW NAME */

		gNewObjectDefinition.coord.x = -kHSTableXSpread;
		ObjNode* text1 = TextMesh_New(gHighScores[i].name, 0, &gNewObjectDefinition);

		if (returnNth == i)
			toReturn = text1;

			/* DRAW SCORE */

		NumToStringC(gHighScores[i].score, scratch);	// convert score to a text string

		gNewObjectDefinition.coord.x = kHSTableXSpread;
		ObjNode* text2 = TextMesh_New(scratch, 2, &gNewObjectDefinition);

		gNewObjectDefinition.coord.y += kHSTableLineHeight;

			/* CHAIN */

		if (chainTail)
		{
			chainTail->ChainNode = text1;
			text1->ChainNode = text2;
			chainTail = text2;
		}
	}

	return toReturn;
}


/********************* SETUP SCORE SCREEN **********************/

static void SetupScoreScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode				*newObj;
Str255				scoreString;

	PlaySong(SONG_HIGHSCORE, true);

	gDrawScoreVerbage = true;
	gExitHighScores = false;
	gFinalScoreAlpha = 1.0f;


		/* IF THIS WAS A SAVED GAME AND SCORE HASN'T CHANGED AND IS ALREADY IN LIST THEN DON'T ADD TO HIGH SCORES */

	if (gPlayingFromSavedGame && (gScore == gLoadedScore) && IsThisScoreInList(gScore))
	{
		gFinalScoreTimer = 4.0f;
		gNewScoreSlot = -1;
	}

			/* NOT SAVED GAME OR A BETTER SCORE THAN WAS LOADED OR ISN'T IN LIST YET */
	else
	{
		gNewScoreSlot = AddNewScore(gScore);				// try to add new score to high scores list
		if (gNewScoreSlot == -1)							// -1 if not added
			gFinalScoreTimer = 4.0f;
		else
			gFinalScoreTimer = 3.0f;
	}



			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.9;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 5000;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.z		= 800;
	viewDef.camera.from.y		= 0;


			/*********************/
			/* SET ANAGLYPH INFO */
			/*********************/

	if (gGamePrefs.anaglyph)
	{
		if (!gGamePrefs.anaglyphColor)
		{
			viewDef.lights.ambientColor.r 		+= .1f;					// make a little brighter
			viewDef.lights.ambientColor.g 		+= .1f;
			viewDef.lights.ambientColor.b 		+= .1f;
		}

		gAnaglyphFocallength	= 180.0f;
		gAnaglyphEyeSeparation 	= 10.0f;
	}

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:highscores.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_HIGHSCORES, gGameViewInfoPtr);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_OTTO, gGameViewInfoPtr);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:particle.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTICLES, gGameViewInfoPtr);
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:highscores.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_HIGHSCORES, gGameViewInfoPtr);



			/************/
			/* MAKE CYC */
			/************/

	gNewObjectDefinition.group 		= MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.type 		= HIGHSCORES_SObjType_Cyc;
	gNewObjectDefinition.coord		= viewDef.camera.from;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= 10;
	gNewObjectDefinition.moveCall 	= MoveHighScoresCyc;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);



	NumToStringC(gScore, scoreString);
	gNewObjectDefinition.flags		= 0;
	gNewObjectDefinition.scale		= 2.0f;
	gNewObjectDefinition.coord.x	= -4;
	gNewObjectDefinition.coord.y	= -10;
	gNewObjectDefinition.coord.z	= 0;
	gNewObjectDefinition.moveCall	= MoveScoreVerbageTally;
	TextMesh_New(scoreString, 1, &gNewObjectDefinition);
}




/********************** FREE SCORE SCREEN **********************/

static void FreeScoreScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	gNewScoreNameMesh = nil;
	gCursorMesh = nil;
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_BONUS);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}



/***************** DRAW HIGHSCORES CALLBACK *******************/

static void DrawHighScoresCallback(OGLSetupOutputType *info)
{
	DrawObjects(info);
	DrawSparkles(info);											// draw light sparkles


			/* DRAW SPRITES */

	OGL_PushState();

	SetHighScoresSpriteState();

	if (gDrawScoreVerbage)
		DrawScoreVerbage(info);
	else
		DrawHighScoresAndCursor(info);


	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;
}


/********************* SET HIGHSCORES SPRITE STATE ***************/

static void SetHighScoresSpriteState(void)
{
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures


			/* INIT MATRICES */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-g2DLogicalWidth*.5f, g2DLogicalWidth*.5f, g2DLogicalHeight*.5f, -g2DLogicalHeight*.5f, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/********************* DRAW SCORE VERBAGE ****************************/

static void DrawScoreVerbage(OGLSetupOutputType *info)
{
				/* SEE IF DONE */

	gFinalScoreTimer -= gFramesPerSecondFrac;
	if (gFinalScoreTimer <= 0.0f)
	{
		if (gNewScoreSlot != -1)							// see if bail or if let player enter name for high score
		{
			StartEnterName();
		}
		else
			gExitHighScores = true;
		return;
	}
	if (gFinalScoreTimer < 1.0f)							// fade out
		gFinalScoreAlpha = gFinalScoreTimer;


			/****************************/
			/* DRAW BONUS TOTAL VERBAGE */
			/****************************/
			//
			// Note: the score itself is an ObjNode so it isn't drawn here.
			//

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gFinalScoreAlpha;
	DrawInfobarSprite2(-150, -70, 300, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_ScoreTextGlow, info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gFinalScoreAlpha;
	DrawInfobarSprite2(-150, -70, 300, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_ScoreText, info);

			/* RESTORE GLOBAL TRANSPARENCY */

	gGlobalTransparency = 1.0f;
}


/****************** DRAW HIGH SCORES AND CURSOR ***********************/

static void DrawHighScoresAndCursor(OGLSetupOutputType *info)
{
	gFinalScoreAlpha += gFramesPerSecondFrac;						// fade in
	if (gFinalScoreAlpha > .99f)
		gFinalScoreAlpha = .99f;


 	gCursorFlux += gFramesPerSecondFrac * 10.0f;


			/****************************/
			/* DRAW ENTER NAME VERBAGE */
			/****************************/

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gFinalScoreAlpha;
	DrawInfobarSprite2(-250, -240+10, 500, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_EnterNameGlow, info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gFinalScoreAlpha;
	DrawInfobarSprite2(-250, -240+10, 500, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_EnterNameText, info);


		/*******************/
		/* DRAW THE CURSOR */
		/*******************/

	if (gCursorIndex < MAX_NAME_LENGTH)						// dont draw if off the right side
	{
		gCursorMesh->ColorFilter.a = (.3f + ((sinf(gCursorFlux) + 1.0f) * .5f) * .699f) * gFinalScoreAlpha;
	}
	else
	{
		gCursorMesh->ColorFilter.a = 0;
	}



			/***********/
			/* CLEANUP */
			/***********/

	gGlobalTransparency = 1;
}



/************************* START ENTER NAME **********************************/

static void StartEnterName(void)
{
	gFinalScoreAlpha = 0;
	gDrawScoreVerbage = false;
	gCursorIndex = 0;
	MyFlushEvents();


	gNewScoreNameMesh = SetupHighScoreTableObjNodes(nil, gNewScoreSlot);


	memset(&gNewObjectDefinition, 0, sizeof(gNewObjectDefinition));
	gNewObjectDefinition.slot		= SLOT_OF_DUMB+100;
	gNewObjectDefinition.moveCall	= nil;
	gNewObjectDefinition.flags		= 0;
	gNewObjectDefinition.scale		= kHSTableScale;
	gNewObjectDefinition.coord.x 	= -kHSTableXSpread;
	gNewObjectDefinition.coord.y 	= kHSTableYStart + kHSTableLineHeight * gNewScoreSlot;
	gNewObjectDefinition.coord.z 	= 0;

	gCursorMesh = TextMesh_New("|", 0, &gNewObjectDefinition);
}



/************************ MOVE HIGH SCORES CYC *******************************/

static void MoveHighScoresCyc(ObjNode *theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac * .07f;
	theNode->Rot.z += gFramesPerSecondFrac * .05f;

	UpdateObjectTransforms(theNode);
}


static void MoveScoreVerbageTally(ObjNode* theNode)
{
	if (!gDrawScoreVerbage)
	{
		DeleteObject(theNode);
		return;
	}

	theNode->ColorFilter.a = (.9f + RandomFloat()*.099f) * gFinalScoreAlpha;
}


#pragma mark -


/*********************** LOAD HIGH SCORES ********************************/

void LoadHighScores(void)
{
OSErr				iErr;
short				refNum;
FSSpec				file;
long				count;

				/* OPEN FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr == fnfErr)
		ClearHighScores();
	else
	if (iErr)
		DoFatalAlert("LoadHighScores: Error opening High Scores file!");
	else
	{
		count = sizeof(HighScoreType) * NUM_SCORES;
		iErr = FSRead(refNum, &count, (Ptr) &gHighScores[0]);				// read data from file
		if (iErr)
		{
			FSClose(refNum);
			FSpDelete(&file);												// file is corrupt, so delete
			return;
		}
		FSClose(refNum);
	}

	// Sanitize scores
	for (int i = 0; i < NUM_SCORES; i++)
	{
		gHighScores[i].name[MAX_NAME_LENGTH] = '\0';
	}
}


/************************ SAVE HIGH SCORES ******************************/

static void SaveHighScores(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

				/* CREATE BLANK FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'Otto', 'Skor', smSystemScript);					// create blank file
	if (iErr)
		goto err;


				/* OPEN FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
err:
		DoAlert("Unable to Save High Scores file!");
		return;
	}

				/* WRITE DATA */

	count = sizeof(HighScoreType) * NUM_SCORES;
	FSWrite(refNum, &count, (Ptr) &gHighScores[0]);
	FSClose(refNum);

}


/**************** CLEAR HIGH SCORES **********************/

void ClearHighScores(void)
{
short				i,j;
char				blank[MAX_NAME_LENGTH] = "EMPTY----------";


			/* INIT SCORES */

	for (i=0; i < NUM_SCORES; i++)
	{
		gHighScores[i].name[0] = MAX_NAME_LENGTH;
		for (j=0; j < MAX_NAME_LENGTH; j++)
			gHighScores[i].name[j+1] = blank[j];
		gHighScores[i].score = 0;
	}

	SaveHighScores();
}


/*************************** ADD NEW SCORE ****************************/
//
// Returns high score slot that score was inserted to or -1 if none
//

static short AddNewScore(uint32_t newScore)
{
short	slot,i;

			/* FIND INSERT SLOT */

	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (newScore > gHighScores[slot].score)
			goto	got_slot;
	}
	return(-1);


got_slot:
			/* INSERT INTO LIST */

	for (i = NUM_SCORES-1; i > slot; i--)						// make hole
		gHighScores[i] = gHighScores[i-1];
	gHighScores[slot].score = newScore;							// set score in structure
	memset(gHighScores[slot].name, 0, sizeof(gHighScores[slot].name));
	return(slot);
}


#pragma mark -

/****************** IS THIS SCORE IN LIST *********************/
//
// Returns True if this score value is anywhere in the high scores already
//

static Boolean IsThisScoreInList(uint32_t score)
{
short	slot;

	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (gHighScores[slot].score == score)
			return(true);
	}

	return(false);
}









