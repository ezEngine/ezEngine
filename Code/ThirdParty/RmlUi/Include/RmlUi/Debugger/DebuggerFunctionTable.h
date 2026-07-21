#pragma once

#include "Header.h"

using ContextPtr = void*;
using InitFunc = bool (*)(ContextPtr);
using ShutdownFunc = void (*)();
using SetContextFunc = bool (*)(ContextPtr);
using SetVisibleFunc = void (*)(bool);
using IsVisibleFunc = bool (*)();

struct Rml_Debugger_Functions
{
  InitFunc m_InitFunc;
  ShutdownFunc m_ShutdownFunc;
  SetContextFunc m_SetContextFunc;
  SetVisibleFunc m_SetVisibleFunc;
  IsVisibleFunc m_IsVisibleFunc;
};

using GetFunctionsFunc = void (*)(Rml_Debugger_Functions*);

extern "C"
{
  RMLUIDEBUGGER_API void Rml_Debugger_GetFunctions(Rml_Debugger_Functions* pFunctions);
}
