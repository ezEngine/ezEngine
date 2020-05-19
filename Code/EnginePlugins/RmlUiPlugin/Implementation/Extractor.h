#pragma once

#include <RmlUi/Core/RenderInterface.h>

namespace ezRmlUiInternal
{
  class Extractor : public Rml::Core::RenderInterface
  {
  public:
    ezRenderData* GetRenderData();
  };
} // namespace ezRmlUiInternal
