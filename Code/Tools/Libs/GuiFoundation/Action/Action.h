#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QKeySequence>
#include <ToolsFoundation/Document/DocumentManager.h>

class QWidget;
struct ezActionDescriptor;
class ezAction;
struct ezActionContext;

using ezActionId = ezGenericId<24, 8>;

/// Creates an instance of an action for the given context. See ezActionDescriptor::CreateAction().
using CreateActionFunc = ezAction* (*)(const ezActionContext&);
/// Destroys an instance created by a CreateActionFunc. If none is given, the action is deleted with the default allocator.
using DeleteActionFunc = void (*)(ezAction*);

/// \brief Handle for a ezActionDescriptor.
///
/// ezAction can be invalidated at runtime so don't store them.
class EZ_GUIFOUNDATION_DLL ezActionDescriptorHandle
{
public:
  using StorageType = ezUInt32;

  EZ_DECLARE_HANDLE_TYPE(ezActionDescriptorHandle, ezActionId);
  friend class ezActionManager;

public:
  const ezActionDescriptor* GetDescriptor() const;
};

/// Determines the range in which an action's shortcut is active and which contexts it needs.
struct ezActionScope
{
  enum Enum
  {
    Global,   ///< Available application wide, independent of any document or window.
    Document, ///< Requires ezActionContext::m_pDocument. Its shortcut is only active while that document's window has focus.
    Window,   ///< Requires ezActionContext::m_pWindow. Its shortcut is only active within that window.
    Default = Global
  };
  using StorageType = ezUInt8;
};

/// What kind of UI element an action maps to when a menu, menu bar or toolbar is built from an action map.
struct ezActionType
{
  enum Enum
  {
    Action,        ///< A single clickable item (menu entry, toolbar button).
    Category,      ///< Groups the items mapped below it, displayed as a separator or a separate toolbar section.
    Menu,          ///< A sub-menu that only holds other items, it cannot be executed itself.
    ActionAndMenu, ///< Can both be executed and opened as a sub-menu, e.g. a toolbar button with an attached drop-down.
    Default = Action
  };
  using StorageType = ezUInt8;
};

/// The environment that an action instance operates on.
///
/// Which members have to be filled out depends on the ezActionScope of the action.
struct EZ_GUIFOUNDATION_DLL ezActionContext
{
  ezActionContext() = default;
  ezActionContext(ezDocument* pDoc) { m_pDocument = pDoc; }

  ezDocument* m_pDocument = nullptr; ///< The document that the action shall affect. Required for ezActionScope::Document.
  ezString m_sMapping;               ///< Name of the ezActionMap from which the UI element was built.
  QWidget* m_pWindow = nullptr;      ///< The widget that the action belongs to. Required for ezActionScope::Window.
};


/// Describes a type of action, from which any number of action instances can be created.
///
/// Descriptors are registered once (see ezActionManager and the EZ_REGISTER_ACTION macros) and hold everything
/// that is shared between all instances, such as the name and the configured shortcut. Refer to them through
/// ezActionDescriptorHandle rather than by pointer.
struct EZ_GUIFOUNDATION_DLL ezActionDescriptor
{
  ezActionDescriptor() = default;
  ;
  ezActionDescriptor(ezActionType::Enum type, ezActionScope::Enum scope, const char* szName, const char* szCategoryPath, const char* szShortcut,
    CreateActionFunc createAction, DeleteActionFunc deleteAction = nullptr);

  ezActionDescriptorHandle m_Handle; ///< Set by ezActionManager during registration.
  ezEnum<ezActionType> m_Type;

  ezEnum<ezActionScope> m_Scope;
  ezString m_sActionName;   ///< Unique within category path, shown in key configuration dialog
  ezString m_sCategoryPath; ///< Category in key configuration dialog, e.g. "Tree View" or "File"

  ezString m_sShortcut;        ///< The currently configured shortcut. May be modified by the user, empty means no shortcut.
  ezString m_sDefaultShortcut; ///< The shortcut that the action was registered with, used to reset m_sShortcut.

  /// Creates an action instance for the given context and adds it to GetCreatedActions().
  ///
  /// The result must be destroyed through DeleteAction(), not deleted directly. Usually only called by the
  /// view classes that build menus and toolbars from an action map.
  ezAction* CreateAction(const ezActionContext& context) const;

  /// Destroys an instance that was returned by CreateAction().
  void DeleteAction(ezAction* pAction) const;

  /// Makes all existing instances broadcast their status update event, e.g. after the shortcut was reconfigured.
  void UpdateExistingActions();

  /// The action instances that currently exist for this descriptor.
  ///
  /// One descriptor can have any number of live instances, because the same action may be mapped into
  /// several windows, menus and toolbars at once, each with its own context. State such as
  /// ezButtonAction::IsEnabled() lives on these instances, not on the descriptor, so this is the only
  /// way to observe an action's current state without creating an instance.
  ezArrayPtr<ezAction* const> GetCreatedActions() const { return m_CreatedActions; }

private:
  CreateActionFunc m_CreateAction;
  DeleteActionFunc m_DeleteAction;

  mutable ezHybridArray<ezAction*, 4> m_CreatedActions;
};



/// Base class for all actions, meaning commands that can be triggered through menus, toolbars or shortcuts.
///
/// An instance is always tied to one ezActionContext and is created through its ezActionDescriptor.
/// Derived classes are typically not instantiated directly, see ezButtonAction, ezCategoryAction, ezMenuAction and others.
class EZ_GUIFOUNDATION_DLL ezAction : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAction, ezReflectedClass);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAction);

public:
  ezAction(const ezActionContext& context) { m_Context = context; }

  /// Performs whatever the action does.
  ///
  /// The meaning of the value depends on the concrete action, for buttons it is typically the new checked state,
  /// for others it is an invalid variant.
  virtual void Execute(const ezVariant& value) = 0;

  /// Recomputes the action's enabled/visible state.
  ///
  /// Action proxies are cached and reused, so an action whose state depends on something outside its
  /// context (such as the asset browser selection) cannot compute it once in its constructor.
  /// Callers that build a menu from such actions have to call this before showing it.
  virtual void RefreshState() {}

  /// Broadcasts m_StatusUpdateEvent, so that the UI element displaying this action updates itself.
  void TriggerUpdate();

  const ezActionContext& GetContext() const { return m_Context; }
  ezActionDescriptorHandle GetDescriptorHandle() { return m_hDescriptorHandle; }

public:
  ezEvent<ezAction*> m_StatusUpdateEvent; ///< Fire when the state of the action changes (enabled, value etc...)

protected:
  ezActionContext m_Context;

private:
  friend struct ezActionDescriptor;
  ezActionDescriptorHandle m_hDescriptorHandle;
};
