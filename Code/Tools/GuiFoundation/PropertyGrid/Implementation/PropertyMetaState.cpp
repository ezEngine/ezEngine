#include <PCH.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

EZ_IMPLEMENT_SINGLETON(ezPropertyMetaState);

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyMetaState)

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezPropertyMetaState);
  }

  ON_CORE_SHUTDOWN
  {
    if (ezPropertyMetaState::GetSingleton())
    {
      auto ptr = ezPropertyMetaState::GetSingleton();
      EZ_DEFAULT_DELETE(ptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION

ezPropertyMetaState::ezPropertyMetaState()
  : m_SingletonRegistrar(this)
{

}

void ezPropertyMetaState::GetPropertyState(const ezDocumentObject* pObject, ezMap<ezString, ezPropertyUiState>& out_PropertyStates)
{
  ezPropertyMetaStateEvent eventData;
  eventData.m_pPropertyStates = &out_PropertyStates;
  eventData.m_pObject = pObject;

  m_Events.Broadcast(eventData);
}

void ezPropertyMetaState::GetPropertyState(const ezHybridArray<ezPropertySelection, 8>& items, ezMap<ezString, ezPropertyUiState>& out_PropertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp.Clear();
    GetPropertyState(sel.m_pObject, m_Temp);

    for (auto it = m_Temp.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_PropertyStates[it.Key()];

      curState.m_Visibility = ezMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
      curState.m_bIsDefaultValue &= it.Value().m_bIsDefaultValue;
    }
  }
}



