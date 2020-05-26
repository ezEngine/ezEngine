#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class ezRmlUiContext;
struct ezMsgExtractRenderData;

namespace ezRmlUiInternal
{
  class Extractor;
  class FileInterface;
  class SystemInterface;
} // namespace ezRmlUiInternal

class EZ_RMLUIPLUGIN_DLL ezRmlUi
{
  EZ_DECLARE_SINGLETON(ezRmlUi);

public:
  ezRmlUi();
  ~ezRmlUi();

  ezRmlUiContext* CreateContext(const char* szName, const ezVec2U32& initialSize);
  void DeleteContext(ezRmlUiContext* pContext);

  void ExtractContext(ezRmlUiContext& context, ezMsgExtractRenderData& msg);

private:
  ezMutex m_ExtractionMutex;
  ezUniquePtr<ezRmlUiInternal::Extractor> m_pExtractor;

  ezUniquePtr<ezRmlUiInternal::FileInterface> m_pFileInterface;
  ezUniquePtr<ezRmlUiInternal::SystemInterface> m_pSystemInterface;

  ezDynamicArray<ezUniquePtr<ezRmlUiContext>> m_Contexts;
};
