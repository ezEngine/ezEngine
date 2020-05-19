#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class ezRmlUiContext;

namespace ezRmlUiInternal
{
  class FileInterface;
  class SystemInterface;
} // namespace ezRmlUiInternal

class EZ_RMLUIPLUGIN_DLL ezRmlUi
{
  EZ_DECLARE_SINGLETON(ezRmlUi);

public:
  ezRmlUi();
  ~ezRmlUi();

  ezRmlUiContext* CreateContext();
  void DeleteContext(ezRmlUiContext* pContext);

private:
  ezUniquePtr<ezRmlUiInternal::FileInterface> m_pFileInterface;
  ezUniquePtr<ezRmlUiInternal::SystemInterface> m_pSystemInterface;

  ezDynamicArray<ezUniquePtr<ezRmlUiContext>> m_Contexts;
};
