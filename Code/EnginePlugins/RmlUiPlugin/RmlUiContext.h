#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <RmlUi/Core.h>

namespace ezRmlUiInternal
{
  class Extractor;
}

class EZ_RMLUIPLUGIN_DLL ezRmlUiContext
{
private:
  friend class ezRmlUi;
  friend class ezMemoryUtils;

  ezRmlUiContext(const char* szName, const ezVec2U32& initialSize);
  ~ezRmlUiContext();

public:
  ezResult LoadDocumentFromFile(const char* szFile);
  ezResult LoadDocumentFromString(const ezStringView& sContent);

  void Update();

  void SetOffset(const ezVec2I32& offset);
  void SetSize(const ezVec2U32& size);
  void SetDpiScale(float fScale);

  Rml::Core::Context* GetRmlContext();

private:
  void ExtractRenderData(ezRmlUiInternal::Extractor& extractor);

  Rml::Core::Context* m_pContext = nullptr;
  Rml::Core::ElementDocument* m_pDocument = nullptr;

  ezVec2I32 m_Offset = ezVec2I32::ZeroVector();

  ezUInt64 m_uiExtractedFrame = 0;
  ezRenderData* m_pRenderData = nullptr;
};
