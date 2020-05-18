#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

#include <RmlUi/Core.h>

namespace ezRmlUiInternal
{
  class SystemInterface;
}

class EZ_RMLUIPLUGIN_DLL ezRmlUi
{
  EZ_DECLARE_SINGLETON(ezRmlUi);

public:
  ezRmlUi();
  ~ezRmlUi();

  ezUniquePtr<ezRmlUiInternal::SystemInterface> m_pSystemInterface;
};
