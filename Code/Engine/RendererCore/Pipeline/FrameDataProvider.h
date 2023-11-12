#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class EZ_RENDERERCORE_DLL ezFrameDataProviderBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFrameDataProviderBase, ezReflectedClass);

protected:
  ezFrameDataProviderBase();

  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) = 0;

  void* GetData(const ezRenderViewContext& renderViewContext);

private:
  friend class ezRenderPipeline;

  const ezRenderPipeline* m_pOwnerPipeline = nullptr;
  void* m_pData = nullptr;
  ezUInt64 m_uiLastUpdateFrame = 0;
};

template <typename T>
class ezFrameDataProvider : public ezFrameDataProviderBase
{
public:
  T* GetData(const ezRenderViewContext& renderViewContext) { return static_cast<T*>(ezFrameDataProviderBase::GetData(renderViewContext)); }
};
