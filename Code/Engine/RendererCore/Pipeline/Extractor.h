#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class EZ_RENDERERCORE_DLL ezExtractor
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezExtractor);

public:
  ezExtractor() {}
  virtual ~ezExtractor() {}

  virtual void Extract(const ezView& view) = 0;
};


class EZ_RENDERERCORE_DLL ezVisibleObjectsExtractor : public ezExtractor
{
public:
  ezVisibleObjectsExtractor() {}
  ~ezVisibleObjectsExtractor() {}

  virtual void Extract(const ezView& view) override;
};
