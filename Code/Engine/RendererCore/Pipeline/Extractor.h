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

  virtual void Extract(const ezView& view) override;
};

class EZ_RENDERERCORE_DLL ezSelectedObjectsExtractor : public ezExtractor
{
public:
  ezSelectedObjectsExtractor();

  virtual void Extract(const ezView& view) override;

  ezRenderPassType m_OverridePassType;
  const ezDeque<ezGameObjectHandle>* m_pSelection;
};
