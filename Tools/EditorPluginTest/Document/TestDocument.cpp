#include <PCH.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestDocument::ezTestDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath, EZ_DEFAULT_NEW(ezTestObjectManager))
{
  GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezTestDocument::SelectionManagerEventHandler, this));

}

void ezTestDocument::InitializeAfterLoading()
{
  ezDocumentBase::InitializeAfterLoading();

  m_TranslateGizmo.SetDocumentGuid(GetGuid());
  m_TranslateGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezTestDocument::TransformationGizmoEventHandler, this));
}

ezTestDocument::~ezTestDocument()
{
  m_TranslateGizmo.m_BaseEvents.RemoveEventHandler(ezMakeDelegate(&ezTestDocument::TransformationGizmoEventHandler, this));
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
      m_TranslateGizmo.SetVisible(false);
    }
    break;

  case ezSelectionManager::Event::Type::SelectionSet:
  case ezSelectionManager::Event::Type::ObjectAdded:
    {
      m_TranslateGizmo.SetVisible(true);
      
      if (GetSelectionManager()->GetSelection()[0]->GetTypeAccessor().GetType() == ezRTTI::FindTypeByName("ezGameObject"))
      {
        ezVec3 vPos = GetSelectionManager()->GetSelection()[0]->GetTypeAccessor().GetValue("Position").ConvertTo<ezVec3>();
        ezMat4 mt;
        mt.SetTranslationMatrix(vPos);

        m_TranslateGizmo.SetTransformation(mt);
      }
    }
    break;
  }
}

void ezTestDocument::TransformationGizmoEventHandler(const ezGizmoBase::BaseEvent& e)
{
  switch (e.m_Type)
  {
  case ezGizmoBase::BaseEvent::Type::BeginInteractions:
    {
      GetCommandHistory()->BeginTemporaryCommands();

    }
    break;

  case ezGizmoBase::BaseEvent::Type::EndInteractions:
    {
      GetCommandHistory()->EndTemporaryCommands(false);
    }
    break;

  case ezGizmoBase::BaseEvent::Type::Interaction:
    {
      const ezMat4 mTransform = e.m_pGizmo->GetTransformation();

      auto Selection = GetSelectionManager()->GetSelection();

      GetCommandHistory()->StartTransaction();

      bool bCancel = false;

      ezSetObjectPropertyCommand cmd;
      cmd.m_bEditorProperty = false;
      cmd.m_NewValue = mTransform.GetTranslationVector();
      cmd.SetPropertyPath("Position");

      auto hType = ezRTTI::FindTypeByName("ezGameObject");

      for (ezUInt32 sel = 0; sel < Selection.GetCount(); ++sel)
      {
        if (!Selection[sel]->GetTypeAccessor().GetType()->IsDerivedFrom(hType))
          continue;

        cmd.m_Object = Selection[sel]->GetGuid();

        ezStatus res = GetCommandHistory()->AddCommand(cmd);

        //ezUIServices::GetInstance()->MessageBoxStatus(res, "Failed to set the position");

        if (res.m_Result.Failed())
        {
          bCancel = true;
          break;
        }
      }

      GetCommandHistory()->EndTransaction(bCancel);
    }
    break;
  }

}

