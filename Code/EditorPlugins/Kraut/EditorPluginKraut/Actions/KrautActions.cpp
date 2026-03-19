#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezKrautActions::s_hCategory;
ezActionDescriptorHandle ezKrautActions::s_hWindStrengthMenu;
ezActionDescriptorHandle ezKrautActions::s_hWindStrength[4];
ezActionDescriptorHandle ezKrautActions::s_hToggleFrondsLeaves;

void ezKrautActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("KrautCategory");

  s_hWindStrengthMenu = EZ_REGISTER_MENU_WITH_ICON("Kraut.Wind.Menu", ":/EditorPluginKraut/Wind.svg");
  s_hWindStrength[0] = EZ_REGISTER_ACTION_2(
    "Kraut.Wind.Off", ezActionScope::Document, "Kraut Tree", "", ezKrautAction, ezKrautAction::ActionType::WindStrength, ezKrautWindStrength::Off);
  s_hWindStrength[1] = EZ_REGISTER_ACTION_2(
    "Kraut.Wind.Light", ezActionScope::Document, "Kraut Tree", "", ezKrautAction, ezKrautAction::ActionType::WindStrength, ezKrautWindStrength::Light);
  s_hWindStrength[2] = EZ_REGISTER_ACTION_2(
    "Kraut.Wind.Moderate", ezActionScope::Document, "Kraut Tree", "", ezKrautAction, ezKrautAction::ActionType::WindStrength, ezKrautWindStrength::Moderate);
  s_hWindStrength[3] = EZ_REGISTER_ACTION_2(
    "Kraut.Wind.Strong", ezActionScope::Document, "Kraut Tree", "", ezKrautAction, ezKrautAction::ActionType::WindStrength, ezKrautWindStrength::Strong);

  s_hToggleFrondsLeaves = EZ_REGISTER_ACTION_1("Kraut.ToggleFrondsLeaves", ezActionScope::Document, "Kraut Tree", "", ezKrautAction, ezKrautAction::ActionType::ToggleFrondsLeaves);
}

void ezKrautActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hWindStrengthMenu);

  for (int i = 0; i < EZ_ARRAY_SIZE(s_hWindStrength); ++i)
    ezActionManager::UnregisterAction(s_hWindStrength[i]);

  ezActionManager::UnregisterAction(s_hToggleFrondsLeaves);
}

void ezKrautActions::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "KrautCategory";

  pMap->MapAction(s_hWindStrengthMenu, szSubPath, 1.0f);

  ezStringBuilder sSubPath(szSubPath, "/Kraut.Wind.Menu");

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hWindStrength); ++i)
    pMap->MapAction(s_hWindStrength[i], sSubPath, i + 1.0f);

  pMap->MapAction(s_hToggleFrondsLeaves, szSubPath, 2.0f);
}

ezKrautAction::ezKrautAction(const ezActionContext& context, const char* szName, ezKrautAction::ActionType type, ezKrautWindStrength::Enum windStrength)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_WindStrength = windStrength;

  m_pDocument = const_cast<ezKrautTreeAssetDocument*>(static_cast<const ezKrautTreeAssetDocument*>(context.m_pDocument));
  m_pDocument->m_Events.AddEventHandler(ezMakeDelegate(&ezKrautAction::KrautEventHandler, this));

  UpdateState();

  if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    SetIconPath(":/EditorPluginKraut/Leaf.svg");
  }
}

ezKrautAction::~ezKrautAction()
{
  m_pDocument->m_Events.RemoveEventHandler(ezMakeDelegate(&ezKrautAction::KrautEventHandler, this));
}

void ezKrautAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::WindStrength)
  {
    m_pDocument->SetWindStrength(m_WindStrength);
  }
  else if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    m_pDocument->SetShowFrondsLeaves(!m_pDocument->GetShowFrondsLeaves());
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

void ezKrautAction::KrautEventHandler(const ezKrautTreeAssetEvent& e)
{
  switch (e.m_Type)
  {
    case ezKrautTreeAssetEvent::Type::WindStrengthChanged:
    case ezKrautTreeAssetEvent::Type::FrondsLeavesVisibilityChanged:
      UpdateState();
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void ezKrautAction::UpdateState()
{
  if (m_Type == ActionType::WindStrength)
  {
    SetCheckable(true);
    SetChecked(m_pDocument->GetWindStrength() == m_WindStrength);
  }
  else if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    SetCheckable(true);
    SetChecked(m_pDocument->GetShowFrondsLeaves());
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
}
