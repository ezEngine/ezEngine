#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_IMPLEMENT_SINGLETON(ezPropertyMetaState);

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyMetaState)

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezPropertyMetaState);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (ezPropertyMetaState::GetSingleton())
    {
      auto ptr = ezPropertyMetaState::GetSingleton();
      EZ_DEFAULT_DELETE(ptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezPropertyMetaState::ezPropertyMetaState()
  : m_SingletonRegistrar(this)
{
}

void ezPropertyMetaState::GetTypePropertiesState(const ezDocumentObject* pObject, ezMap<ezString, ezPropertyUiState>& out_PropertyStates)
{
  ezPropertyMetaStateEvent eventData;
  eventData.m_pPropertyStates = &out_PropertyStates;
  eventData.m_pObject = pObject;

  m_Events.Broadcast(eventData);
}

void ezPropertyMetaState::GetTypePropertiesState(const ezHybridArray<ezPropertySelection, 8>& items, ezMap<ezString, ezPropertyUiState>& out_PropertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp.Clear();
    GetTypePropertiesState(sel.m_pObject, m_Temp);

    for (auto it = m_Temp.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_PropertyStates[it.Key()];

      curState.m_Visibility = ezMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
    }
  }
}

void ezPropertyMetaState::GetContainerElementsState(const ezDocumentObject* pObject, const char* szProperty, ezHashTable<ezVariant, ezPropertyUiState>& out_PropertyStates)
{
  ezContainerElementMetaStateEvent eventData;
  eventData.m_pContainerElementStates = &out_PropertyStates;
  eventData.m_pObject = pObject;
  eventData.m_szProperty = szProperty;

  m_ContainerEvents.Broadcast(eventData);
}

void ezPropertyMetaState::GetContainerElementsState(const ezHybridArray<ezPropertySelection, 8>& items, const char* szProperty, ezHashTable<ezVariant, ezPropertyUiState>& out_PropertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp2.Clear();
    GetContainerElementsState(sel.m_pObject, szProperty, m_Temp2);

    for (auto it = m_Temp2.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_PropertyStates[it.Key()];

      curState.m_Visibility = ezMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
    }
  }
}

