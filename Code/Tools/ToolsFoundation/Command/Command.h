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
  EZ_ADD_DYNAMIC_REFLECTION(ezCommand, ezReflectedClass);

public:
  ezCommand();

  bool IsUndoable() const { return m_bUndoable; };
  bool HasChildActions() const { return !m_ChildActions.IsEmpty(); }

  enum class CommandState { WasDone, WasUndone };

protected:
  ezStatus Do(bool bRedo);
  ezStatus Undo(bool bFireEvents);
  void Cleanup(CommandState state);

  ezStatus AddSubCommand(ezCommand& command);
  ezDocument* GetDocument() { return m_pDocument; };

private:
  virtual bool HasReturnValues() const { return false; }
  virtual ezStatus DoInternal(bool bRedo) = 0;
  virtual ezStatus UndoInternal(bool bFireEvents) = 0;
  virtual void CleanupInternal(CommandState state) = 0;

protected:
  friend class ezCommandHistory;
  friend class ezCommandTransaction;

  ezString m_sDescription; // TODO
  bool m_bUndoable; // TODO
  ezHybridArray<ezCommand*, 8> m_ChildActions;
  ezDocument* m_pDocument;
};
