#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>

#include <RmlUi/Core.h>

class EZ_RMLUIPLUGIN_DLL ezRmlUi
{
  EZ_DECLARE_SINGLETON(ezRmlUi);

public:
  ezRmlUi();
  ~ezRmlUi();
};
