#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>

//class ezDocumentTypeBase;
//
//struct EZ_TOOLSFOUNDATION_DLL ezContextChange
//{
//  const ezDocumentTypeBase* m_pOldContext;
//  const ezDocumentTypeBase* m_pNewContext;
//};
//
///// \brief Tracks existing and active ezDocumentTypeBase.
//class EZ_TOOLSFOUNDATION_DLL ezDocumentTypeRegistry
//{
//public:
//  static bool RegisterDocumentType(const ezDocumentTypeBase* pContext);
//  static bool UnregisterDocumentType(const ezDocumentTypeBase* pContext);
//
//  static ezArrayPtr<const ezDocumentTypeBase*> GetContexts() { return s_Contexts; }
//
//  static void SetActiveContext(const ezDocumentTypeBase* pContext);
//  static const ezDocumentTypeBase* GetActiveContext();
//
//private:
//  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, ContextRegistry);
//
//  static void Startup();
//  static void Shutdown();
//
//public:
//  static ezEvent<ezContextChange&> m_ContextAddedEvent;
//  static ezEvent<ezContextChange&> m_ContextRemovedEvent;
//  static ezEvent<ezContextChange&> m_ActiveContextChanged;
//
//private:
//  static ezHybridArray<const ezDocumentTypeBase*, 8> s_Contexts;
//  static ezDocumentTypeBase* s_pActiveContext;
//};
