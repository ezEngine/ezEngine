#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class EZ_RENDERERCORE_DLL ezExtractor : public ezReflectedClass
{
public:
  EZ_ADD_DYNAMIC_REFLECTION(ezExtractor, ezReflectedClass);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezExtractor);

public:
  ezExtractor() {}
  virtual ~ezExtractor() {}

  /// \brief Sets the name of the extractor.
  void SetName(const char* szName);

  /// \brief returns the name of the extractor.
  const char* GetName() const;

  virtual void Extract(const ezView& view, ezRenderPipeline* pRenderPipeline) {};

private:
  ezHashedString m_sName;
};


class EZ_RENDERERCORE_DLL ezVisibleObjectsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisibleObjectsExtractor, ezExtractor);
public:
  ezVisibleObjectsExtractor() {}

  virtual void Extract(const ezView& view, ezRenderPipeline* pRenderPipeline) override;
};

class EZ_RENDERERCORE_DLL ezSelectedObjectsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectedObjectsExtractor, ezExtractor);
public:
  ezSelectedObjectsExtractor();

  virtual void Extract(const ezView& view, ezRenderPipeline* pRenderPipeline) override;
  virtual const ezDeque<ezGameObjectHandle>* GetSelection() = 0;
  ezRenderPassType m_OverridePassType;
  //const ezDeque<ezGameObjectHandle>* m_pSelection;
};

class EZ_RENDERERCORE_DLL ezCallDelegateExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCallDelegateExtractor, ezExtractor);
public:
  ezCallDelegateExtractor();

  virtual void Extract(const ezView& view, ezRenderPipeline* pRenderPipeline) override;

  ezDelegate<void ()> m_Delegate;
};