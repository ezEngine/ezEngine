#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <RmlUi/Core.h>

class EZ_RMLUIPLUGIN_DLL ezRmlUiContext
{
private:
  friend class ezRmlUi;
  friend class ezMemoryUtils;

  ezRmlUiContext();
  ~ezRmlUiContext();

public:
  ezResult LoadDocumentFromFile(const char* szFile);
  ezResult LoadDocumentFromString(const ezStringView& sContent);

  void Update();

  Rml::Core::Context* GetRmlContext();

private:
  Rml::Core::Context* m_pContext = nullptr;
  Rml::Core::ElementDocument* m_pDocument = nullptr;

  ezUInt64 m_uiExtractedFrame = 0;
  ezRenderData* m_pRenderData = nullptr;
};
