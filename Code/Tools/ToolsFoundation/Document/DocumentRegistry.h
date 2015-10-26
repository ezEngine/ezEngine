#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <ToolsFoundation/Document/Document.h>

struct EZ_TOOLSFOUNDATION_DLL ezActiveDocumentChange
{
  const ezDocument* m_pOldDocument;
  const ezDocument* m_pNewDocument;
};

/// \brief Tracks existing and active ezDocument.
///
/// While the IDocumentManager manages documents of a certain context,
/// this class simply keeps track of the overall number of documents and the currently active one.
class EZ_TOOLSFOUNDATION_DLL ezDocumentRegistry
{
public:
  static bool RegisterDocument(const ezDocument* pDocument);
  static bool UnregisterDocument(const ezDocument* pDocument);

  static ezArrayPtr<const ezDocument*> GetDocuments() { return s_Documents; }

  static void SetActiveDocument(const ezDocument* pDocument);
  static const ezDocument* GetActiveDocument();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, DocumentRegistry);

  static void Startup();
  static void Shutdown();

public:
  //static ezEvent<ezDocumentChange&> m_DocumentAddedEvent;
  //static ezEvent<ezDocumentChange&> m_DocumentRemovedEvent;
  static ezEvent<ezActiveDocumentChange&> m_ActiveDocumentChanged;

private:
  static ezHybridArray<const ezDocument*, 16> s_Documents;
  static ezDocument* s_pActiveDocument;
};
