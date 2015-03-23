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

struct ezActionDescriptor;
class ezAction;
struct ezActionContext;

typedef ezGenericId<24, 8> ezActionId;
typedef ezAction* (*CreateActionFunc)(const ezActionContext& context);
typedef void (*DeleteActionFunc)(ezAction* pAction);

/// \brief Handle for a ezAction.
///
/// ezAction can be invalidated at runtime so don't store them.
class EZ_GUIFOUNDATION_DLL ezActionHandle
{
public:
  typedef ezUInt32 StorageType;
  EZ_DECLARE_HANDLE_TYPE(ezActionHandle, ezActionId);
  friend class ezActionManager;
public:
  const ezActionDescriptor* GetDescriptor() const;
};

///
struct ActionScope
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
struct ActionType
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
  ezUuid m_Document;
  ezHashedString m_sWindow;
  ezHashedString m_sMapping;
};


///
struct EZ_GUIFOUNDATION_DLL ezActionDescriptor
{
  ezActionDescriptor() {};
  ezActionDescriptor(ActionType::Enum type, ActionScope::Enum scope, const char* szName, ezHashedString sCategoryPath,
    CreateActionFunc createAction, DeleteActionFunc deleteAction);

  ezActionHandle m_Handle;
  ezEnum<ActionType> m_Type;
  ezEnum<ActionScope> m_Scope;
  ezString m_sActionName; ///< Unique within category path, shown in key configuration dialog
  ezHashedString m_sCategoryPath; ///< Category in key configuration dialog, e.g. "Tree View" or "File"
  
  // Default shortcut

  CreateActionFunc m_CreateAction;
  DeleteActionFunc m_DeleteAction;
};



///
class EZ_GUIFOUNDATION_DLL ezAction : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAction);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAction);
public:
  ezAction() {}
  virtual ezResult Init(const ezActionContext& context) = 0;
  virtual ezResult Execute(const ezVariant& value) = 0;

public:
  ezEvent<ezAction*> m_StatusUpdateEvent; ///< Fire when the state of the action changes (enabled, value etc...)
};


