#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSceneDocument::ezSceneDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath, EZ_DEFAULT_NEW(ezSceneObjectManager))
{
  

}

void ezSceneDocument::InitializeAfterLoading()
{
  ezDocumentBase::InitializeAfterLoading();

}

ezSceneDocument::~ezSceneDocument()
{
}

ezStatus ezSceneDocument::InternalSaveDocument()
{
  return ezDocumentBase::InternalSaveDocument();
}

