#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Basics/Guid.h>
#include <Foundation/Reflection/Reflection.h>

class ezDocumentBase;
class ezDocumentTypeBase;

/// \brief Interface for a command
///
/// Commands are the only objects that have non-const access to any data structures (contexts, documents etc.).
/// Thus, any modification must go through a command and the ezCommandHistory is the only class capable of executing commands.
class EZ_TOOLSFOUNDATION_DLL ezCommandBase : public ezReflectedClass
{
public:
  virtual bool IsUndoable() const;
  
private:
  virtual bool Do();
  virtual bool Undo();

protected:
  ezDocumentBase* GetDocument(const ezGuid& documentGuid);
  ezDocumentTypeBase* GetContext(const ezGuid& contextGuid);
};
