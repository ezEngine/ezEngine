#include <PCH.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGreyBoxEditTool, 1, ezRTTIDefaultAllocator<ezGreyBoxEditTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezGreyBoxEditTool::ezGreyBoxEditTool()
{
  m_DrawBoxGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezGreyBoxEditTool::GizmoEventHandler, this));
}

ezGreyBoxEditTool::~ezGreyBoxEditTool()
{
  m_DrawBoxGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezGreyBoxEditTool::GizmoEventHandler, this));
}

ezEditorInputContext* ezGreyBoxEditTool::GetEditorInputContextOverride()
{
  if (IsActive())
    return &m_DrawBoxGizmo;

  return nullptr;
}

ezEditToolSupportedSpaces ezGreyBoxEditTool::GetSupportedSpaces() const
{
  return ezEditToolSupportedSpaces::WorldSpaceOnly;
}

bool ezGreyBoxEditTool::GetSupportsMoveParentOnly() const
{
  return false;
}


void ezGreyBoxEditTool::GetGridSettings(ezGridSettingsMsgToEngine& msg)
{
  auto pSceneDoc = GetDocument();
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument());

  msg.m_fGridDensity = ezSnapProvider::GetTranslationSnapValue(); // negative density = local space
  msg.m_vGridTangent1.SetZero(); // indicates that the grid is disabled
  msg.m_vGridTangent2.SetZero(); // indicates that the grid is disabled

  if (pPreferences->GetShowGrid())
  {
    if (m_DrawBoxGizmo.GetCurrentMode() == ezDrawBoxGizmo::ManipulateMode::DrawBase)
    {
      msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

      msg.m_vGridTangent1 = ezVec3(1, 0, 0);
      msg.m_vGridTangent2 = ezVec3(0, 1, 0);
    }
    else if (m_DrawBoxGizmo.GetCurrentMode() == ezDrawBoxGizmo::ManipulateMode::DrawHeight)
    {
      const ezVec3 vCamDir = GetWindow()->GetFocusedViewWidget()->m_pViewConfig->m_Camera.GetDirForwards();

      //float dummy;
      //m_DrawBoxGizmo.GetResult(msg.m_vGridCenter, dummy, dummy, dummy, dummy, dummy, dummy);

      msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

      if (ezMath::Abs(ezVec3(1, 0, 0).Dot(vCamDir)) < ezMath::Abs(ezVec3(0, 1, 0).Dot(vCamDir)))
      {
        msg.m_vGridTangent1 = ezVec3(1, 0, 0);
        msg.m_vGridTangent2 = ezVec3(0, 0, 1);
      }
      else
      {
        msg.m_vGridTangent1 = ezVec3(0, 1, 0);
        msg.m_vGridTangent2 = ezVec3(0, 0, 1);
      }
    }
  }
}

void ezGreyBoxEditTool::UpdateGizmoState()
{
  ezManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);

  m_DrawBoxGizmo.SetVisible(IsActive());
  m_DrawBoxGizmo.SetTransformation(ezTransform::Identity());
}

void ezGreyBoxEditTool::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
  case ezGameObjectEvent::Type::ActiveEditToolChanged:
    UpdateGizmoState();
    break;
  }
}

void ezGreyBoxEditTool::ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() && !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void ezGreyBoxEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezGreyBoxEditTool::GameObjectEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezGreyBoxEditTool::ManipulatorManagerEventHandler, this));

  m_DrawBoxGizmo.SetOwner(GetWindow(), nullptr);
}

void ezGreyBoxEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_DrawBoxGizmo.UpdateStatusBarText(GetWindow());
  }
}

void ezGreyBoxEditTool::GizmoEventHandler(const ezGizmoEvent& e)
{
  if (e.m_Type == ezGizmoEvent::Type::EndInteractions)
  {
    ezVec3 vCenter;
    float negx, posx, negy, posy, negz, posz;
    m_DrawBoxGizmo.GetResult(vCenter, negx, posx, negy, posy, negz, posz);

    auto* pDoc = GetDocument();
    auto* pHistory = pDoc->GetCommandHistory();

    pHistory->StartTransaction("Add Grey-Box");

    ezUuid objGuid, compGuid;
    objGuid.CreateNewUuid();
    compGuid.CreateNewUuid();

    {
      ezAddObjectCommand cmdAdd;
      cmdAdd.m_NewObjectGuid = objGuid;
      cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
      cmdAdd.m_sParentProperty = "Children";
      pHistory->AddCommand(cmdAdd);
    }
    {
      ezSetObjectPropertyCommand cmdPos;
      cmdPos.m_NewValue = vCenter;
      cmdPos.m_Object = objGuid;
      cmdPos.m_sProperty = "LocalPosition";
      pHistory->AddCommand(cmdPos);
    }
    {
      ezAddObjectCommand cmdComp;
      cmdComp.m_NewObjectGuid = compGuid;
      cmdComp.m_pType = ezRTTI::FindTypeByName("ezGreyBoxComponent");
      cmdComp.m_sParentProperty = "Components";
      cmdComp.m_Parent = objGuid;
      cmdComp.m_Index = -1;
      pHistory->AddCommand(cmdComp);
    }

    {
      ezSetObjectPropertyCommand cmdSize;
      cmdSize.m_Object = compGuid;

      cmdSize.m_NewValue = negx;
      cmdSize.m_sProperty = "SizeNegX";
      pHistory->AddCommand(cmdSize);

      cmdSize.m_NewValue = posx;
      cmdSize.m_sProperty = "SizePosX";
      pHistory->AddCommand(cmdSize);

      cmdSize.m_NewValue = negy;
      cmdSize.m_sProperty = "SizeNegY";
      pHistory->AddCommand(cmdSize);

      cmdSize.m_NewValue = posy;
      cmdSize.m_sProperty = "SizePosY";
      pHistory->AddCommand(cmdSize);

      cmdSize.m_NewValue = negz;
      cmdSize.m_sProperty = "SizeNegZ";
      pHistory->AddCommand(cmdSize);

      cmdSize.m_NewValue = posz;
      cmdSize.m_sProperty = "SizePosZ";
      pHistory->AddCommand(cmdSize);
    }

    pHistory->FinishTransaction();

    pDoc->GetSelectionManager()->SetSelection(pDoc->GetObjectManager()->GetObject(objGuid));
  }
}

