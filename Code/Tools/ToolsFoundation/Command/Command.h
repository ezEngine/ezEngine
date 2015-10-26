#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Basics/Status.h>
#include <Foundation/Reflection/Reflection.h>

class ezDocument;
class ezCommandTransaction;
class ezJSONWriter;
class ezJSONReader;

/// \brief Interface for a command
///
/// Commands are the only objects that have non-const access to any data structures (contexts, documents etc.).
/// Thus, any modification must go through a command and the ezCommandHistory is the only class capable of executing commands.
class EZ_TOOLSFOUNDATION_DLL ezCommand : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCommand);

public:
  ezCommand();

  bool IsUndoable() const { return m_bUndoable; };

  enum class CommandState { WasDone, WasUndone };

private:

  virtual ezStatus Do(bool bRedo) = 0;
  virtual ezStatus Undo(bool bFireEvents) = 0;
  virtual void Cleanup(CommandState state) = 0;

protected:
  ezDocument* GetDocument() { return m_pDocument; };

protected:
  friend class ezCommandHistory;
  friend class ezCommandTransaction;

  ezString m_sDescription; // TODO
  bool m_bUndoable; // TODO

private:
  ezDocument* m_pDocument;
};
