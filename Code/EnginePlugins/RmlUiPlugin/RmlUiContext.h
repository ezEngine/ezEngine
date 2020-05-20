#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <RmlUi/Core.h>

struct ezMsgExtractRenderData;

namespace ezRmlUiInternal
{
  class Extractor;
} // namespace ezRmlUiInternal


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

  void ExtractRenderData(ezMsgExtractRenderData& msg);

  Rml::Core::Context* GetRmlContext();

private:
  Rml::Core::Context* m_pContext = nullptr;
  Rml::Core::ElementDocument* m_pDocument = nullptr;

  ezUniquePtr<ezRmlUiInternal::Extractor> m_pExtractor;
  ezMutex m_ExtractionMutex;
  ezUInt64 m_uiExtractedFrame = 0;
};
