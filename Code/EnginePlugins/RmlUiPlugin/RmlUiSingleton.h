#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class ezRmlUiContext;
class ezOpenDdlWriter;
class ezOpenDdlReaderElement;
struct ezMsgExtractRenderData;

namespace ezRmlUiInternal
{
  class Extractor;
  class FileInterface;
  class SystemInterface;
} // namespace ezRmlUiInternal

/// \brief The fmod configuration to be used on a specific platform
struct EZ_RMLUIPLUGIN_DLL ezRmlUiConfiguration
{
  ezDynamicArray<ezString> m_Fonts;

  ezResult Save(const char* szFile) const;
  ezResult Load(const char* szFile);

  bool operator==(const ezRmlUiConfiguration& rhs) const;
  bool operator!=(const ezRmlUiConfiguration& rhs) const { return !operator==(rhs); }
};

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

  ezRmlUiConfiguration m_Config;

  ezDynamicArray<ezUniquePtr<ezRmlUiContext>> m_Contexts;  
};
