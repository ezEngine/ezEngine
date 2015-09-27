#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/Map.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <QKeySequence>

class QWidget;
struct ezActionDescriptor;
class ezAction;
struct ezActionContext;

typedef ezGenericId<24, 8> ezActionId;
typedef ezAction* (*CreateActionFunc)(const ezActionContext& context);
typedef void (*DeleteActionFunc)(ezAction* pAction);

/// \brief Handle for a ezAction.
///
/// ezAction can be invalidated at runtime so don't store them.
class EZ_GUIFOUNDATION_DLL ezActionDescriptorHandle
{
public:
  typedef ezUInt32 StorageType;
  EZ_DECLARE_HANDLE_TYPE(ezActionDescriptorHandle, ezActionId);
  friend class ezActionManager;
public:
  const ezActionDescriptor* GetDescriptor() const;
};

///
struct ezActionScope
{
  enum Enum
  {
    Global,
    Document,
    Window,
    Default = Global
  };
  typedef ezUInt8 StorageType;
};

///
struct ezActionType
{
  enum Enum
  {
    Action,
    Category,
    Menu,
    Default = Action
  };
  typedef ezUInt8 StorageType;
};

///
struct EZ_GUIFOUNDATION_DLL ezActionContext
{
  ezActionContext()
    : m_pDocument(nullptr), m_pWindow(nullptr) {}

  ezDocumentBase* m_pDocument;
  ezString m_sMapping;
  QWidget* m_pWindow;
};


///
struct EZ_GUIFOUNDATION_DLL ezActionDescriptor
{
  ezActionDescriptor() {};
  ezActionDescriptor(ezActionType::Enum type, ezActionScope::Enum scope, const char* szName, const char* szCategoryPath, const char* szShortcut,
    CreateActionFunc createAction, DeleteActionFunc deleteAction = nullptr);

  ezActionDescriptorHandle m_Handle;
  ezEnum<ezActionType> m_Type;

  ezEnum<ezActionScope> m_Scope;
  ezString m_sActionName; ///< Unique within category path, shown in key configuration dialog
  ezString m_sCategoryPath; ///< Category in key configuration dialog, e.g. "Tree View" or "File"
  
  ezString m_sShortcut;

  ezAction* CreateAction(const ezActionContext& context) const;
  void DeleteAction(ezAction* pAction) const;

private:
  CreateActionFunc m_CreateAction;
  DeleteActionFunc m_DeleteAction;
};



///
class EZ_GUIFOUNDATION_DLL ezAction : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAction);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAction);
public:
  ezAction(const ezActionContext& context) { m_Context = context; }
  virtual void Execute(const ezVariant& value) = 0;

  void TriggerUpdate();
  const ezActionContext& GetContext() const { return m_Context; }
  ezActionDescriptorHandle GetDescriptorHandle() { return m_DescriptorHandle; }

public:
  ezEvent<ezAction*> m_StatusUpdateEvent; ///< Fire when the state of the action changes (enabled, value etc...)

protected:
  ezActionContext m_Context;

private:
  friend struct ezActionDescriptor;
  ezActionDescriptorHandle m_DescriptorHandle;
};


