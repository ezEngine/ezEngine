#include <PCH.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorFramework/EditorApp.moc.h>
#include <Core/World/GameObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestDocument::ezTestDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath, new ezTestObjectManager(this))
{
  GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezTestDocument::SelectionManagerEventHandler, this));

  m_Gizmo.SetDocumentGuid(GetGuid());
}

ezTestDocument::~ezTestDocument()
{
  GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezTestDocument::SelectionManagerEventHandler, this));
}

ezStatus ezTestDocument::InternalSaveDocument()
{
  return ezDocumentBase::InternalSaveDocument();
}

void ezTestDocument::SelectionManagerEventHandler(const ezSelectionManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManager::Event::Type::SelectionCleared:
    {
      m_Gizmo.SetVisible(false);
      m_Gizmo.SetTransformation(ezMat4::IdentityMatrix());
    }
    break;

  case ezSelectionManager::Event::Type::SelectionSet:
    {
      m_Gizmo.SetVisible(true);
      
      if (GetSelectionManager()->GetSelection()[0]->GetTypeAccessor().GetReflectedTypeHandle().GetType()->GetTypeName().GetString() == "ezGameObject")
      {
        ezMat4 m;
        m.SetTranslationMatrix(ezVec3(1, 1, 1));
        m_Gizmo.SetTransformation(m);
      }
    }
    break;
  }
}

