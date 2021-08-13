#pragma once

enum
{
	kTextMeshAlignLeft,
	kTextMeshAlignCenter,
	kTextMeshAlignRight,
};

void TextMesh_Init(OGLSetupOutputType* setupInfo, bool redFont);
void TextMesh_Shutdown(void);
ObjNode* TextMesh_NewEmpty(int capacity, NewObjectDefinitionType *newObjDef);
ObjNode* TextMesh_New(const char *text, int align, NewObjectDefinitionType *newObjDef);
void TextMesh_Update(const char* text, int align, ObjNode* textNode);
float TextMesh_GetCharX(const char* text, int n);
OGLRect TextMesh_GetExtents(ObjNode* textNode);
void TextMesh_DrawExtents(ObjNode* textNode);
