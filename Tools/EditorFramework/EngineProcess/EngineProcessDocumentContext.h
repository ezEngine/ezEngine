#pragma once

#include <EditorFramework/Plugin.h>
#include <Core/World/World.h>
#include <Foundation/Types/Uuid.h>

class ezEditorEngineSyncObjectMsg;
class ezEditorEngineSyncObject;

/// \brief A document context is the counter part to an editor document on the engine side.
///
/// For every document in the editor that requires engine output (rendering, picking, etc.), there is a ezEngineProcessDocumentContext
/// created in the engine process.
class EZ_EDITORFRAMEWORK_DLL ezEngineProcessDocumentContext
{
public:
  ezEngineProcessDocumentContext() { }
  virtual ~ezEngineProcessDocumentContext() { }

  static ezEngineProcessDocumentContext* GetDocumentContext(ezUuid guid);
  static void AddDocumentContext(ezUuid guid, ezEngineProcessDocumentContext* pView);
  static void DestroyDocumentContext(ezUuid guid);

  void ProcessEditorEngineSyncObjectMsg(const ezEditorEngineSyncObjectMsg& msg);

  ezWorld* m_pWorld;

private:
  // Maps a document guid to the corresponding context that handles that document on the engine side
  static ezHashTable<ezUuid, ezEngineProcessDocumentContext*> s_DocumentContexts;

  ezHashTable<ezUuid, ezEditorEngineSyncObject*> m_pSyncObjects;
};

