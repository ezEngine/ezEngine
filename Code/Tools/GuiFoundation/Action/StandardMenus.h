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
    View = EZ_BIT(2),
    Settings = EZ_BIT(3),
    Help = EZ_BIT(4),
    //Window
    Default = 0
  };

  struct Bits
  {
    StorageType File : 1;
    StorageType Edit : 1;
    StorageType View : 1;
    StorageType Settings : 1;
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
  static ezActionDescriptorHandle s_hMenuView;
  static ezActionDescriptorHandle s_hMenuSettings;
  static ezActionDescriptorHandle s_hMenuHelp;
};