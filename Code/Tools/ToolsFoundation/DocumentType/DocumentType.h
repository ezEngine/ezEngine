#pragma once

#include <ToolsFoundation/Basics.h>

class ezCommandRegistry;

/// \brief A context is the central hub class for a certain type of editor.
///
/// The context contains a document manager to handle the open documents of its type and various
/// functions for handling files it is responsible for.
class EZ_TOOLSFOUNDATION_DLL ezDocumentTypeBase
{
public:
  virtual ~ezDocumentTypeBase() { }



  const ezCommandRegistry* GetCommandRegistry() const;


private:
  ezCommandRegistry* m_pCommandRegistry;

};
