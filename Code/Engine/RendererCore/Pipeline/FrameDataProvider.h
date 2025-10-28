#pragma once

#include <RendererCore/Pipeline/Declarations.h>

/// Base class for frame data providers.
///
/// Frame data providers supply per-frame data to the rendering pipeline (e.g., clustered light data).
/// The data is computed once per frame and cached. Derived classes implement
/// UpdateData() to create or update the data. The pipeline calls GetData() to retrieve it.
class EZ_RENDERERCORE_DLL ezFrameDataProviderBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFrameDataProviderBase, ezReflectedClass);

protected:
  ezFrameDataProviderBase();

  /// Derived classes implement this to create or update frame data.
  ///
  /// Called once per frame when the data is first requested. Returns a pointer to the data.
  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) = 0;

  /// Returns the cached frame data, updating it if necessary.
  void* GetData(const ezRenderViewContext& renderViewContext);

private:
  friend class ezRenderPipeline;

  const ezRenderPipeline* m_pOwnerPipeline = nullptr;
  void* m_pData = nullptr;
  ezUInt64 m_uiLastUpdateFrame = 0;
};

/// Typed frame data provider template.
///
/// Simplifies creating frame data providers by providing type-safe access to the data.
template <typename T>
class ezFrameDataProvider : public ezFrameDataProviderBase
{
public:
  T* GetData(const ezRenderViewContext& renderViewContext) { return static_cast<T*>(ezFrameDataProviderBase::GetData(renderViewContext)); }
};
