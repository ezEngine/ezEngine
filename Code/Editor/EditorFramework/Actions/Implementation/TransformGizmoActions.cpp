#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezGizmoAction::ezGizmoAction(const ezActionContext& context, const char* szName, const ezRTTI* pGizmoType)
  : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_pGizmoType = pGizmoType;
  m_pGameObjectDocument = static_cast<ezGameObjectDocument*>(context.m_pDocument);
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezGizmoAction::GameObjectEventHandler, this));

  if (m_pGizmoType)
  {
    ezStringBuilder sIcon(":/TypeIcons/", m_pGizmoType->GetTypeName(), ".svg");
    SetIconPath(sIcon);
  }
  else
  {
    SetIconPath(":/EditorFramework/Icons/GizmoNone.svg");
  }

  UpdateState();
}

ezGizmoAction::~ezGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezGizmoAction::GameObjectEventHandler, this));
}

void ezGizmoAction::Execute(const ezVariant& value)
{
  m_pGameObjectDocument->SetActiveEditTool(m_pGizmoType);
  UpdateState();
}

void ezGizmoAction::UpdateState()
{
  SetChecked(m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType));
}

void ezGizmoAction::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  if (e.m_Type == ezGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

//////////////////////////////////////////////////////////////////////////

ezToggleWorldSpaceGizmo::ezToggleWorldSpaceGizmo(const ezActionContext& context, const char* szName, const ezRTTI* pGizmoType)
  : ezGizmoAction(context, szName, pGizmoType)
{
}

void ezToggleWorldSpaceGizmo::Execute(const ezVariant& value)
{
  if (m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType))
  {
    // toggle local/world space if the same tool is selected again
    m_pGameObjectDocument->SetGizmoWorldSpace(!m_pGameObjectDocument->GetGizmoWorldSpace());
  }
  else
  {
    ezGizmoAction::Execute(value);
  }
}

//////////////////////////////////////////////////////////////////////////

class ezSnapTranslationMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSnapTranslationMenuAction, ezDynamicMenuAction);

public:
  ezSnapTranslationMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath)
    : ezDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezSnapTranslationMenuAction::SnapEvent, this));
  }

  ~ezSnapTranslationMenuAction()
  {
    ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSnapTranslationMenuAction::SnapEvent, this));
  }

  void SnapEvent(const ezSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = ezSnapProvider::GetTranslationSnapValue();

    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.0f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.0");
      e.m_UserValue = 0.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.01f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.0_01");
      e.m_UserValue = 0.01f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.05f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.0_05");
      e.m_UserValue = 0.05f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.0_1");
      e.m_UserValue = 0.1f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.2f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.0_2");
      e.m_UserValue = 0.2f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.25f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.0_25");
      e.m_UserValue = 0.25f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.5f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.0_5");
      e.m_UserValue = 0.5f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 1.0f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.1");
      e.m_UserValue = 1.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 2.0f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.2");
      e.m_UserValue = 2.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 4.0f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.4");
      e.m_UserValue = 4.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 5.0f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.5");
      e.m_UserValue = 5.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 8.0f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.8");
      e.m_UserValue = 8.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 10.0f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Translate.Snap.10");
      e.m_UserValue = 10.0f;
    }
  }

  virtual void Execute(const ezVariant& value) override
  {
    ezSnapProvider::SetTranslationSnapValue(value.Get<float>());
  };

  void UpdateIcon()
  {
    const float fValue = ezSnapProvider::GetTranslationSnapValue();

    if (fValue == 0.0f)
      SetIconPath(":EditorFramework/Icons/Snap0cm.svg");
    else if (fValue == 0.01f)
      SetIconPath(":EditorFramework/Icons/Snap1cm.svg");
    else if (fValue == 0.05f)
      SetIconPath(":EditorFramework/Icons/Snap5cm.svg");
    else if (fValue == 0.1f)
      SetIconPath(":EditorFramework/Icons/Snap10cm.svg");
    else if (fValue == 0.2f)
      SetIconPath(":EditorFramework/Icons/Snap20cm.svg");
    else if (fValue == 0.25f)
      SetIconPath(":EditorFramework/Icons/Snap25cm.svg");
    else if (fValue == 0.5f)
      SetIconPath(":EditorFramework/Icons/Snap50cm.svg");
    else if (fValue == 1.0f)
      SetIconPath(":EditorFramework/Icons/Snap100cm.svg");
    else if (fValue == 2.0f)
      SetIconPath(":EditorFramework/Icons/Snap200cm.svg");
    else if (fValue == 4.0f)
      SetIconPath(":EditorFramework/Icons/Snap400cm.svg");
    else if (fValue == 5.0f)
      SetIconPath(":EditorFramework/Icons/Snap500cm.svg");
    else if (fValue == 8.0f)
      SetIconPath(":EditorFramework/Icons/Snap800cm.svg");
    else if (fValue == 10.0f)
      SetIconPath(":EditorFramework/Icons/Snap1000cm.svg");

    TriggerUpdate();
  }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSnapTranslationMenuAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

class ezSnapRotationMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSnapRotationMenuAction, ezDynamicMenuAction);

public:
  ezSnapRotationMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath)
    : ezDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezSnapRotationMenuAction::SnapEvent, this));
  }

  ~ezSnapRotationMenuAction()
  {
    ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSnapRotationMenuAction::SnapEvent, this));
  }

  void SnapEvent(const ezSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = ezSnapProvider::GetRotationSnapValue().GetDegree();

    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 0.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Rotation.Snap.0_Degree");
      e.m_UserValue = 0.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 1.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Rotation.Snap.1_Degree");
      e.m_UserValue = 1.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 5.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Rotation.Snap.5_Degree");
      e.m_UserValue = 5.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 10.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Rotation.Snap.10_Degree");
      e.m_UserValue = 10.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 15.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Rotation.Snap.15_Degree");
      e.m_UserValue = 15.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 22.5f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Rotation.Snap.22_5_Degree");
      e.m_UserValue = 22.5f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 30.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Rotation.Snap.30_Degree");
      e.m_UserValue = 30.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 45.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Rotation.Snap.45_Degree");
      e.m_UserValue = 45.0f;
    }
  }

  virtual void Execute(const ezVariant& value) override
  {
    ezSnapProvider::SetRotationSnapValue(ezAngle::MakeFromDegree(value.Get<float>()));
  };

  void UpdateIcon()
  {
    const float fValue = ezSnapProvider::GetRotationSnapValue().GetDegree();

    if (ezMath::IsEqual(fValue, 0.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap0deg.svg");
    else if (ezMath::IsEqual(fValue, 1.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap1deg.svg");
    else if (ezMath::IsEqual(fValue, 5.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap5deg.svg");
    else if (ezMath::IsEqual(fValue, 10.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap10deg.svg");
    else if (ezMath::IsEqual(fValue, 15.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap15deg.svg");
    else if (ezMath::IsEqual(fValue, 22.5f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap22deg.svg");
    else if (ezMath::IsEqual(fValue, 30.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap30deg.svg");
    else if (ezMath::IsEqual(fValue, 45.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap45deg.svg");

    TriggerUpdate();
  }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSnapRotationMenuAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

class ezSnapScaleMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSnapScaleMenuAction, ezDynamicMenuAction);

public:
  ezSnapScaleMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath)
    : ezDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezSnapScaleMenuAction::SnapEvent, this));
  }

  ~ezSnapScaleMenuAction()
  {
    ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSnapScaleMenuAction::SnapEvent, this));
  }

  void SnapEvent(const ezSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = ezSnapProvider::GetScaleSnapValue();

    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 0.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Scale.Snap.0");
      e.m_UserValue = 0.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 0.125f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Scale.Snap.0_125");
      e.m_UserValue = 0.125f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 0.25f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Scale.Snap.0_25");
      e.m_UserValue = 0.25f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 0.5f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Scale.Snap.0_5");
      e.m_UserValue = 0.5f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 1.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Scale.Snap.1");
      e.m_UserValue = 1.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 2.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Scale.Snap.2");
      e.m_UserValue = 2.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = ezMath::IsEqual(fValue, 4.0f, 0.1f) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = ezTranslate("Gizmo.Scale.Snap.4");
      e.m_UserValue = 4.0f;
    }
  }

  virtual void Execute(const ezVariant& value) override
  {
    ezSnapProvider::SetScaleSnapValue(value.Get<float>());
  };

  void UpdateIcon()
  {
    const float fValue = ezSnapProvider::GetScaleSnapValue();

    if (ezMath::IsEqual(fValue, 0.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap0x.svg");
    else if (ezMath::IsEqual(fValue, 0.125f, 0.05f))
      SetIconPath(":EditorFramework/Icons/Snap0125x.svg");
    else if (ezMath::IsEqual(fValue, 0.25f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap025x.svg");
    else if (ezMath::IsEqual(fValue, 0.5f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap05x.svg");
    else if (ezMath::IsEqual(fValue, 1.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap1x.svg");
    else if (ezMath::IsEqual(fValue, 2.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap2x.svg");
    else if (ezMath::IsEqual(fValue, 4.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap4x.svg");

    TriggerUpdate();
  }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSnapScaleMenuAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezTransformGizmoActions::s_hGizmoCategory;
ezActionDescriptorHandle ezTransformGizmoActions::s_hGizmoMenu;
ezActionDescriptorHandle ezTransformGizmoActions::s_hNoGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hTranslateGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hRotateGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hScaleGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hDragToPositionGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hWorldSpace;
ezActionDescriptorHandle ezTransformGizmoActions::s_hMoveParentOnly;
ezActionDescriptorHandle ezTransformGizmoActions::s_SnapSettings;
ezActionDescriptorHandle ezTransformGizmoActions::s_SnapTranslationMenu;
ezActionDescriptorHandle ezTransformGizmoActions::s_SnapRotationMenu;
ezActionDescriptorHandle ezTransformGizmoActions::s_SnapScaleMenu;

void ezTransformGizmoActions::RegisterActions()
{
  s_hGizmoCategory = EZ_REGISTER_CATEGORY("GizmoCategory");
  s_hGizmoMenu = EZ_REGISTER_MENU("G.Gizmos");
  s_hNoGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Select", ezActionScope::Document, "Gizmo", "Q", ezGizmoAction, nullptr);
  s_hTranslateGizmo = EZ_REGISTER_ACTION_1(
    "Gizmo.Mode.Translate", ezActionScope::Document, "Gizmo", "W", ezToggleWorldSpaceGizmo, ezGetStaticRTTI<ezTranslateGizmoEditTool>());
  s_hRotateGizmo = EZ_REGISTER_ACTION_1(
    "Gizmo.Mode.Rotate", ezActionScope::Document, "Gizmo", "E", ezToggleWorldSpaceGizmo, ezGetStaticRTTI<ezRotateGizmoEditTool>());
  s_hScaleGizmo =
    EZ_REGISTER_ACTION_1("Gizmo.Mode.Scale", ezActionScope::Document, "Gizmo", "R", ezGizmoAction, ezGetStaticRTTI<ezScaleGizmoEditTool>());
  s_hDragToPositionGizmo = EZ_REGISTER_ACTION_1(
    "Gizmo.Mode.DragToPosition", ezActionScope::Document, "Gizmo", "T", ezGizmoAction, ezGetStaticRTTI<ezDragToPositionGizmoEditTool>());
  s_hWorldSpace = EZ_REGISTER_ACTION_1(
    "Gizmo.TransformSpace", ezActionScope::Document, "Gizmo", "X", ezTransformGizmoAction, ezTransformGizmoAction::ActionType::GizmoToggleWorldSpace);
  s_hMoveParentOnly = EZ_REGISTER_ACTION_1("Gizmo.MoveParentOnly", ezActionScope::Document, "Gizmo", "", ezTransformGizmoAction,
    ezTransformGizmoAction::ActionType::GizmoToggleMoveParentOnly);
  s_SnapSettings = EZ_REGISTER_ACTION_1(
    "Gizmo.SnapSettings", ezActionScope::Document, "Gizmo", "End", ezTransformGizmoAction, ezTransformGizmoAction::ActionType::GizmoSnapSettings);

  s_SnapTranslationMenu = EZ_REGISTER_DYNAMIC_MENU("Gizmo.Translation.Snap.Dropdown", ezSnapTranslationMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
  s_SnapRotationMenu = EZ_REGISTER_DYNAMIC_MENU("Gizmo.Rotation.Snap.Dropdown", ezSnapRotationMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
  s_SnapScaleMenu = EZ_REGISTER_DYNAMIC_MENU("Gizmo.Scale.Snap.Dropdown", ezSnapScaleMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
}

void ezTransformGizmoActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hGizmoCategory);
  ezActionManager::UnregisterAction(s_hGizmoMenu);
  ezActionManager::UnregisterAction(s_hNoGizmo);
  ezActionManager::UnregisterAction(s_hTranslateGizmo);
  ezActionManager::UnregisterAction(s_hRotateGizmo);
  ezActionManager::UnregisterAction(s_hScaleGizmo);
  ezActionManager::UnregisterAction(s_hDragToPositionGizmo);
  ezActionManager::UnregisterAction(s_hWorldSpace);
  ezActionManager::UnregisterAction(s_hMoveParentOnly);
  ezActionManager::UnregisterAction(s_SnapSettings);
  ezActionManager::UnregisterAction(s_SnapTranslationMenu);
  ezActionManager::UnregisterAction(s_SnapRotationMenu);
  ezActionManager::UnregisterAction(s_SnapScaleMenu);
}

void ezTransformGizmoActions::MapMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const ezStringView sTarget = "G.Gizmos";

  pMap->MapAction(s_hGizmoMenu, "G.Edit", 4.0f);
  pMap->MapAction(s_hNoGizmo, sTarget, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sTarget, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sTarget, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sTarget, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sTarget, 4.0f);
  pMap->MapAction(s_hWorldSpace, sTarget, 6.0f);
  pMap->MapAction(s_hMoveParentOnly, sTarget, 7.0f);
  pMap->MapAction(s_SnapSettings, sTarget, 8.0f);
}

void ezTransformGizmoActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const ezStringView sSubPath = "GizmoCategory";

  pMap->MapAction(s_hGizmoCategory, "", 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 6.0f);
  pMap->MapAction(s_SnapTranslationMenu, sSubPath, 7.0f);
  pMap->MapAction(s_SnapRotationMenu, sSubPath, 8.0f);
  pMap->MapAction(s_SnapScaleMenu, sSubPath, 9.0f);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransformGizmoAction, 0, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTransformGizmoAction::ezTransformGizmoAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_pGameObjectDocument = static_cast<ezGameObjectDocument*>(context.m_pDocument);

  switch (m_Type)
  {
    case ActionType::GizmoToggleWorldSpace:
      SetIconPath(":/EditorFramework/Icons/WorldSpace.svg");
      break;
    case ActionType::GizmoToggleMoveParentOnly:
      SetIconPath(":/EditorFramework/Icons/TransformParent.svg");
      break;
    case ActionType::GizmoSnapSettings:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/SnapSettings.svg");
      break;
  }

  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezTransformGizmoAction::GameObjectEventHandler, this));
  UpdateState();
}

ezTransformGizmoAction::~ezTransformGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezTransformGizmoAction::GameObjectEventHandler, this));
}

void ezTransformGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    m_pGameObjectDocument->SetGizmoWorldSpace(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    m_pGameObjectDocument->SetGizmoMoveParentOnly(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoSnapSettings)
  {
    ezQtSnapSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  UpdateState();
}

void ezTransformGizmoAction::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  if (e.m_Type == ezGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

void ezTransformGizmoAction::UpdateState()
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    ezGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    SetEnabled(pTool != nullptr && pTool->GetSupportedSpaces() == ezEditToolSupportedSpaces::LocalAndWorldSpace);

    if (pTool != nullptr)
    {
      switch (pTool->GetSupportedSpaces())
      {
        case ezEditToolSupportedSpaces::LocalSpaceOnly:
          SetChecked(false);
          break;
        case ezEditToolSupportedSpaces::WorldSpaceOnly:
          SetChecked(true);
          break;
        case ezEditToolSupportedSpaces::LocalAndWorldSpace:
          SetChecked(m_pGameObjectDocument->GetGizmoWorldSpace());
          break;
      }
    }
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    ezGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    const bool bSupported = pTool != nullptr && pTool->GetSupportsMoveParentOnly();

    SetEnabled(bSupported);
    SetChecked(bSupported && m_pGameObjectDocument->GetGizmoMoveParentOnly());
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmoAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapPivotToGrid;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapObjectsToGrid;

void ezTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_CATEGORY("Gizmo.Translate.Snap.Menu");
  s_hSnapPivotToGrid = EZ_REGISTER_ACTION_1("Gizmo.Translate.Snap.PivotToGrid", ezActionScope::Document, "Gizmo - Position Snap", "Ctrl+End", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SnapSelectionPivotToGrid);
  s_hSnapObjectsToGrid = EZ_REGISTER_ACTION_1("Gizmo.Translate.Snap.ObjectsToGrid", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SnapEachSelectedObjectToGrid);
}

void ezTranslateGizmoAction::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSnappingValueMenu);
  ezActionManager::UnregisterAction(s_hSnapPivotToGrid);
  ezActionManager::UnregisterAction(s_hSnapObjectsToGrid);
}

void ezTranslateGizmoAction::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSnappingValueMenu, "G.Gizmos", 8.0f);

  pMap->MapAction(s_hSnapPivotToGrid, "G.Gizmos", "Gizmo.Translate.Snap.Menu", 0.0f);
  pMap->MapAction(s_hSnapObjectsToGrid, "G.Gizmos", "Gizmo.Translate.Snap.Menu", 1.0f);
}

ezTranslateGizmoAction::ezTranslateGizmoAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_pSceneDocument = static_cast<const ezGameObjectDocument*>(context.m_pDocument);
  m_Type = type;
}

void ezTranslateGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::SnapSelectionPivotToGrid)
    m_pSceneDocument->TriggerSnapPivotToGrid();

  if (m_Type == ActionType::SnapEachSelectedObjectToGrid)
    m_pSceneDocument->TriggerSnapEachObjectToGrid();
}
