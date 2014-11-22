#pragma once

#include <EditorFramework/Plugin.h>
#include <Core/World/World.h>
#include <Foundation/Types/Uuid.h>

class EZ_EDITORFRAMEWORK_DLL ezEngineProcessDocumentContext
{
public:
  ezEngineProcessDocumentContext() { }
  virtual ~ezEngineProcessDocumentContext() { }

  static ezEngineProcessDocumentContext* GetDocumentContext(ezUuid guid);
  static void AddDocumentContext(ezUuid guid, ezEngineProcessDocumentContext* pView);
  static void DestroyDocumentContext(ezUuid guid);

  ezWorld* m_pWorld;

private:
  static ezHashTable<ezUuid, ezEngineProcessDocumentContext*> s_DocumentContexts;

  
};

