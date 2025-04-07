#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationClipAsset/AnimationClipActions.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezAnimationClipActions::s_hCategory;
ezActionDescriptorHandle ezAnimationClipActions::s_hRootMotionFromFeet;

void ezAnimationClipActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("AnimationClipAssetCategory");
  s_hRootMotionFromFeet = EZ_REGISTER_ACTION_1("AnimationClip.RootMotionFromFeet", ezActionScope::Document, "Animation Clip", "", ezAnimationClipAction, ezAnimationClipAction::ActionType::RootMotionFromFeet);
}

void ezAnimationClipActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hRootMotionFromFeet);
}

void ezAnimationClipActions::MapActions(ezStringView sActionMapName, ezStringView sSubPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sActionMapName);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sActionMapName);

  pMap->MapAction(s_hCategory, sSubPath, 20.0f);

  ezStringBuilder sPath;
  sPath.SetPath(sSubPath, "AnimationClipAssetCategory");

  pMap->MapAction(s_hRootMotionFromFeet, sPath, 1.0f);
}

ezAnimationClipAction::ezAnimationClipAction(const ezActionContext& context, const char* szName, ezAnimationClipAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pAssetWindow = static_cast<ezQtAnimationClipAssetDocumentWindow*>(ezQtDocumentWindow::FindWindowByDocument(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::RootMotionFromFeet:
      SetIconPath(":/EditorPluginAssets/Foot.svg");
      break;

    default:
      break;
  }
}

ezAnimationClipAction::~ezAnimationClipAction() = default;

void ezAnimationClipAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RootMotionFromFeet:
      m_pAssetWindow->ExtractRootMotionFromFeet();
      return;
  }
}
