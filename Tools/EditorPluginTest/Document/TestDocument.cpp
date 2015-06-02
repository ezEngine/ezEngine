#include <PCH.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <Core/World/GameObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestDocument::ezTestDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath, EZ_DEFAULT_NEW(ezTestObjectManager))
{
  GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezTestDocument::SelectionManagerEventHandler, this));

}

void ezTestDocument::Initialize()
{
  ezDocumentBase::Initialize();

  m_GizmoX.SetDocumentGuid(GetGuid());
  m_GizmoX.SetColor(ezColorLinearUB(128, 0, 0));

  m_GizmoY.SetDocumentGuid(GetGuid());
  m_GizmoY.SetColor(ezColorLinearUB(0, 128, 0));

  m_GizmoZ.SetDocumentGuid(GetGuid());
  m_GizmoZ.SetColor(ezColorLinearUB(0, 0, 128));
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
      m_GizmoX.SetVisible(false);
      m_GizmoY.SetVisible(false);
      m_GizmoZ.SetVisible(false);
    }
    break;

  case ezSelectionManager::Event::Type::SelectionSet:
  case ezSelectionManager::Event::Type::ObjectAdded:
    {
      m_GizmoX.SetVisible(true);
      m_GizmoY.SetVisible(true);
      m_GizmoZ.SetVisible(true);

      /// \todo Hack / Fix, this should work without it
      m_GizmoX.SetDocumentGuid(GetGuid());
      m_GizmoY.SetDocumentGuid(GetGuid());
      m_GizmoZ.SetDocumentGuid(GetGuid());

      
      if (GetSelectionManager()->GetSelection()[0]->GetTypeAccessor().GetReflectedTypeHandle() == ezReflectedTypeManager::GetTypeHandleByName("ezGameObject"))
      {
        ezVec3 vPos = GetSelectionManager()->GetSelection()[0]->GetTypeAccessor().GetValue("Position").ConvertTo<ezVec3>();
        ezMat4 mt;
        mt.SetTranslationMatrix(vPos);

        ezMat4 m;

        m.SetRotationMatrixZ(ezAngle::Degree(-90));
        m_GizmoX.SetTransformation(mt * m);

        m.SetIdentity();
        m_GizmoY.SetTransformation(mt * m);

        m.SetRotationMatrixX(ezAngle::Degree(90));
        m_GizmoZ.SetTransformation(mt * m);
      }
    }
    break;
  }
}

