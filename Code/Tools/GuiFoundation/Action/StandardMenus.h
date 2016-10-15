#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

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

  static void MapActions(const char* szMapping, const ezBitflags<ezStandardMenuTypes>& Menus);

  static ezActionDescriptorHandle s_hMenuFile;
  static ezActionDescriptorHandle s_hMenuEdit;
  static ezActionDescriptorHandle s_hMenuPanels;
  static ezActionDescriptorHandle s_hMenuProject;
  static ezActionDescriptorHandle s_hMenuScene;
  static ezActionDescriptorHandle s_hMenuView;
  static ezActionDescriptorHandle s_hMenuHelp;
};

///
class EZ_GUIFOUNDATION_DLL ezApplicationPanelsMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezApplicationPanelsMenuAction, ezDynamicMenuAction);
public:
  ezApplicationPanelsMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezDynamicMenuAction(context, szName, szIconPath) {}
  virtual void GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries) override;
  virtual void Execute(const ezVariant& value) override;
};