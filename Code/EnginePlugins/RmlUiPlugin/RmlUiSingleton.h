#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezRmlUiContext;
struct ezMsgExtractRenderData;

/// \brief The RML configuration to be used on a specific platform
struct EZ_RMLUIPLUGIN_DLL ezRmlUiConfiguration
{
  ezDynamicArray<ezString> m_Fonts;

  static constexpr const ezStringView s_sConfigFile = ":project/RuntimeConfigs/RmlUiConfig.ddl"_ezsv;

  ezResult Save(ezStringView sFile = s_sConfigFile) const;
  ezResult Load(ezStringView sFile = s_sConfigFile);

  bool operator==(const ezRmlUiConfiguration& rhs) const;
  bool operator!=(const ezRmlUiConfiguration& rhs) const { return !operator==(rhs); }
};

class EZ_RMLUIPLUGIN_DLL ezRmlUi
{
  EZ_DECLARE_SINGLETON(ezRmlUi);

public:
  ezRmlUi();
  ~ezRmlUi();

  ezRmlUiContext* CreateContext(const char* szName, const ezVec2U32& vInitialSize);
  void DeleteContext(ezRmlUiContext* pContext);

  bool AnyContextWantsInput();

  void ExtractContext(ezRmlUiContext& ref_context, ezGALTextureHandle hTexture);

private:
  struct Data;
  ezUniquePtr<Data> m_pData;
};
