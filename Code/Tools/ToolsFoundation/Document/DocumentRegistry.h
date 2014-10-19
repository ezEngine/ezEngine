#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <ToolsFoundation/Document/Document.h>

struct EZ_TOOLSFOUNDATION_DLL ezActiveDocumentChange
{
  const ezDocumentBase* m_pOldDocument;
  const ezDocumentBase* m_pNewDocument;
};

/// \brief Tracks existing and active ezDocumentBase.
///
/// While the IDocumentManager manages documents of a certain context,
/// this class simply keeps track of the overall number of documents and the currently active one.
class EZ_TOOLSFOUNDATION_DLL ezDocumentRegistry
{
public:
  static bool RegisterDocument(const ezDocumentBase* pDocument);
  static bool UnregisterDocument(const ezDocumentBase* pDocument);

  static ezArrayPtr<const ezDocumentBase*> GetDocuments() { return s_Documents; }

  static void SetActiveDocument(const ezDocumentBase* pDocument);
  static const ezDocumentBase* GetActiveDocument();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, DocumentRegistry);

  static void Startup();
  static void Shutdown();

public:
  //static ezEvent<ezDocumentChange&> m_DocumentAddedEvent;
  //static ezEvent<ezDocumentChange&> m_DocumentRemovedEvent;
  static ezEvent<ezActiveDocumentChange&> m_ActiveDocumentChanged;

private:
  static ezHybridArray<const ezDocumentBase*, 16> s_Documents;
  static ezDocumentBase* s_pActiveDocument;
};
