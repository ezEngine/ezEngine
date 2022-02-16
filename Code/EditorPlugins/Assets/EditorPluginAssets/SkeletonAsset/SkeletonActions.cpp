#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonActions.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezSkeletonActions::s_hCategory;
ezActionDescriptorHandle ezSkeletonActions::s_hRenderBones;
ezActionDescriptorHandle ezSkeletonActions::s_hRenderColliders;
ezActionDescriptorHandle ezSkeletonActions::s_hRenderJoints;
ezActionDescriptorHandle ezSkeletonActions::s_hRenderSwingLimits;
ezActionDescriptorHandle ezSkeletonActions::s_hRenderTwistLimits;

void ezSkeletonActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("SkeletonCategory");
  s_hRenderBones = EZ_REGISTER_ACTION_1("Skeleton.RenderBones", ezActionScope::Document, "Skeletons", "", ezSkeletonAction, ezSkeletonAction::ActionType::RenderBones);
  s_hRenderColliders = EZ_REGISTER_ACTION_1("Skeleton.RenderColliders", ezActionScope::Document, "Skeletons", "", ezSkeletonAction, ezSkeletonAction::ActionType::RenderColliders);
  s_hRenderJoints = EZ_REGISTER_ACTION_1("Skeleton.RenderJoints", ezActionScope::Document, "Skeletons", "", ezSkeletonAction, ezSkeletonAction::ActionType::RenderJoints);
  s_hRenderSwingLimits = EZ_REGISTER_ACTION_1("Skeleton.RenderSwingLimits", ezActionScope::Document, "Skeletons", "", ezSkeletonAction, ezSkeletonAction::ActionType::RenderSwingLimits);
  s_hRenderTwistLimits = EZ_REGISTER_ACTION_1("Skeleton.RenderTwistLimits", ezActionScope::Document, "Skeletons", "", ezSkeletonAction, ezSkeletonAction::ActionType::RenderTwistLimits);
}

void ezSkeletonActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hRenderBones);
  ezActionManager::UnregisterAction(s_hRenderColliders);
  ezActionManager::UnregisterAction(s_hRenderJoints);
  ezActionManager::UnregisterAction(s_hRenderSwingLimits);
  ezActionManager::UnregisterAction(s_hRenderTwistLimits);
}

void ezSkeletonActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "SkeletonCategory";

  pMap->MapAction(s_hRenderBones, szSubPath, 1.0f);
  pMap->MapAction(s_hRenderColliders, szSubPath, 2.0f);
  //pMap->MapAction(s_hRenderJoints, szSubPath, 3.0f);
  pMap->MapAction(s_hRenderSwingLimits, szSubPath, 4.0f);
  pMap->MapAction(s_hRenderTwistLimits, szSubPath, 5.0f);
}

ezSkeletonAction::ezSkeletonAction(const ezActionContext& context, const char* szName, ezSkeletonAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_SkeletonpDocument = const_cast<ezSkeletonAssetDocument*>(static_cast<const ezSkeletonAssetDocument*>(context.m_pDocument));
  m_SkeletonpDocument->Events().AddEventHandler(ezMakeDelegate(&ezSkeletonAction::AssetEventHandler, this));

  switch (m_Type)
  {
    case ActionType::RenderBones:
      SetIconPath(":/EditorPluginAssets/SkeletonBones.png");
      break;

    case ActionType::RenderColliders:
      SetIconPath(":/EditorPluginAssets/SkeletonColliders.png");
      break;

    case ActionType::RenderJoints:
      SetIconPath(":/EditorPluginAssets/SkeletonJoints.png");
      break;

    case ActionType::RenderSwingLimits:
      SetIconPath(":/EditorPluginAssets/JointSwingLimits.png");
      break;

    case ActionType::RenderTwistLimits:
      SetIconPath(":/EditorPluginAssets/JointTwistLimits.png");
      break;
  }

  UpdateState();
}

ezSkeletonAction::~ezSkeletonAction()
{
  m_SkeletonpDocument->Events().RemoveEventHandler(ezMakeDelegate(&ezSkeletonAction::AssetEventHandler, this));
}

void ezSkeletonAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RenderBones:
      m_SkeletonpDocument->SetRenderBones(!m_SkeletonpDocument->GetRenderBones());
      return;

    case ActionType::RenderColliders:
      m_SkeletonpDocument->SetRenderColliders(!m_SkeletonpDocument->GetRenderColliders());
      return;

    case ActionType::RenderJoints:
      m_SkeletonpDocument->SetRenderJoints(!m_SkeletonpDocument->GetRenderJoints());
      return;

    case ActionType::RenderSwingLimits:
      m_SkeletonpDocument->SetRenderSwingLimits(!m_SkeletonpDocument->GetRenderSwingLimits());
      return;

    case ActionType::RenderTwistLimits:
      m_SkeletonpDocument->SetRenderTwistLimits(!m_SkeletonpDocument->GetRenderTwistLimits());
      return;
  }
}

void ezSkeletonAction::AssetEventHandler(const ezSkeletonAssetEvent& e)
{
  switch (e.m_Type)
  {
    case ezSkeletonAssetEvent::RenderStateChanged:
      UpdateState();
      break;
  }
}

void ezSkeletonAction::UpdateState()
{
  if (m_Type == ActionType::RenderBones)
  {
    SetCheckable(true);
    SetChecked(m_SkeletonpDocument->GetRenderBones());
  }

  if (m_Type == ActionType::RenderColliders)
  {
    SetCheckable(true);
    SetChecked(m_SkeletonpDocument->GetRenderColliders());
  }

  if (m_Type == ActionType::RenderJoints)
  {
    SetCheckable(true);
    SetChecked(m_SkeletonpDocument->GetRenderJoints());
  }

  if (m_Type == ActionType::RenderSwingLimits)
  {
    SetCheckable(true);
    SetChecked(m_SkeletonpDocument->GetRenderSwingLimits());
  }

  if (m_Type == ActionType::RenderTwistLimits)
  {
    SetCheckable(true);
    SetChecked(m_SkeletonpDocument->GetRenderTwistLimits());
  }
}
