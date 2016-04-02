#include <GuiFoundation/PCH.h>
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

  m_Events.Broadcast(eventData);

  /// \todo stuff
  if (pObject->GetTypeAccessor().GetType() == ezRTTI::FindTypeByName("ezMeshAssetProperties"))
  {
    ezInt64 primType = pObject->GetTypeAccessor().GetValue("Primitive Type").ConvertTo<ezInt64>();

    if (primType == 0)
    {
      out_PropertyStates["Radius"].m_State = ezPropertyUiState::Invisible;
      out_PropertyStates["Radius 2"].m_State = ezPropertyUiState::Invisible;
      out_PropertyStates["Height"].m_State = ezPropertyUiState::Disabled;
      out_PropertyStates["Detail"].m_State = ezPropertyUiState::Disabled;
    }
  }
}

void ezPropertyMetaState::GetPropertyState(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items, ezMap<ezString, ezPropertyUiState>& out_PropertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp.Clear();
    GetPropertyState(sel.m_pObject, m_Temp);

    for (auto it = m_Temp.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_PropertyStates[it.Key()];

      curState.m_State = ezMath::Max(curState.m_State, it.Value().m_State);
    }
  }
}



