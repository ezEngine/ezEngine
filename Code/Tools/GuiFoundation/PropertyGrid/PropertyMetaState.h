#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

struct ezPropertyUiState
{
  enum State
  {
    Default,
    Invisible,
    Disabled,
  };

  ezPropertyUiState()
  {
    m_State = State::Default;
  }

  State m_State;
};

struct ezPropertyMetaStateEvent
{
  const ezDocumentObject* m_pObject;
  const ezPropertyPath m_PropertyPath;

  ezMap<ezString, ezPropertyUiState>* m_pPropertyStates;
};

class EZ_GUIFOUNDATION_DLL ezPropertyMetaState
{
  EZ_DECLARE_SINGLETON(ezPropertyMetaState);

public:

  ezPropertyMetaState();

  void GetPropertyState(const ezDocumentObject* pObject, ezMap<ezString, ezPropertyUiState>& out_PropertyStates);

  void GetPropertyState(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items, ezMap<ezString, ezPropertyUiState>& out_PropertyStates);
  
  
  ezEvent<ezPropertyMetaStateEvent&> m_Events;

private:
  ezMap<ezString, ezPropertyUiState> m_Temp;
};

