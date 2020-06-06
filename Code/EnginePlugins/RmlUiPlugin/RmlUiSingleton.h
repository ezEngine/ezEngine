#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class ezRmlUiContext;
struct ezMsgExtractRenderData;

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
  struct Data;
  ezUniquePtr<Data> m_pData;
};
