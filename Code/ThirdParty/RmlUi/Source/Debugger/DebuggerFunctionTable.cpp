#include "../../Include/RmlUi/Debugger/Debugger.h"
#include "../../Include/RmlUi/Debugger/DebuggerFunctionTable.h"

namespace
{
  bool InitWrapper(ContextPtr context)
  {
    return Rml::Debugger::Initialise(static_cast<Rml::Context*>(context));
  }

  void ShutdownWrapper()
  {
    Rml::Debugger::Shutdown();
  }

  bool SetContextWrapper(ContextPtr context)
  {
    return Rml::Debugger::SetContext(static_cast<Rml::Context*>(context));
  }

  void SetVisibleWrapper(bool visible)
  {
    Rml::Debugger::SetVisible(visible);
  }

  bool IsVisibleWrapper()
  {
    return Rml::Debugger::IsVisible();
  }
}


void Rml_Debugger_GetFunctions(Rml_Debugger_Functions* pFunctions)
{
  pFunctions->m_InitFunc = InitWrapper;
  pFunctions->m_ShutdownFunc = ShutdownWrapper;
  pFunctions->m_SetContextFunc = SetContextWrapper;
  pFunctions->m_SetVisibleFunc = SetVisibleWrapper;
  pFunctions->m_IsVisibleFunc = IsVisibleWrapper;
}
