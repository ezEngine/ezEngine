#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

struct ezStandardMenuTypes
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    File = EZ_BIT(0),
    Edit = EZ_BIT(1),
    Panels = EZ_BIT(2),
    Project = EZ_BIT(3),
    Scene = EZ_BIT(4),
    View = EZ_BIT(5),
    Help = EZ_BIT(6),

    Default = 0
  };

  struct Bits
  {
    StorageType File : 1;
    StorageType Edit : 1;
    StorageType Panels : 1;
    StorageType Project : 1;
    StorageType Scene : 1;
    StorageType View : 1;
    StorageType Help : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezStandardMenuTypes);

///
class EZ_GUIFOUNDATION_DLL ezStandardMenus
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const ezBitflags<ezStandardMenuTypes>& menus);

  static ezActionDescriptorHandle s_hMenuFile;
  static ezActionDescriptorHandle s_hMenuEdit;
  static ezActionDescriptorHandle s_hMenuPanels;
  static ezActionDescriptorHandle s_hMenuProject;
  static ezActionDescriptorHandle s_hMenuScene;
  static ezActionDescriptorHandle s_hMenuView;
  static ezActionDescriptorHandle s_hMenuHelp;
  static ezActionDescriptorHandle s_hCheckForUpdates;
  static ezActionDescriptorHandle s_hReportProblem;
};

///
class EZ_GUIFOUNDATION_DLL ezApplicationPanelsMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezApplicationPanelsMenuAction, ezDynamicMenuAction);

public:
  ezApplicationPanelsMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath)
    : ezDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_entries) override;
  virtual void Execute(const ezVariant& value) override;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GUIFOUNDATION_DLL ezHelpActions : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezHelpActions, ezButtonAction);

public:
  enum class ButtonType
  {
    CheckForUpdates,
    ReportProblem,
  };

  ezHelpActions(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezHelpActions();

  virtual void Execute(const ezVariant& value) override;

private:
  ButtonType m_ButtonType;
};
