#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Delegate.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezGALCommandEncoder;
class ezGALDevice;
class ezRenderContext;

/// Provided to the pass execution callback. Gives access to the command encoder and allows resolving transient handles to real GPU resources.
///
/// For render passes, BeginRendering has already been called before the callback is invoked and EndRendering will be called after it returns.
class EZ_RENDERERCORE_DLL ezRenderGraphContext
{
public:
  ezRenderGraphContext() = default;
  ezRenderGraphContext(ezGALCommandEncoder* pCommandEncoder, ezGALDevice* pDevice, ezRenderContext* pRenderContext, const ezReflectedClass* pUserData = nullptr)
    : m_pCommandEncoder(pCommandEncoder)
    , m_pDevice(pDevice)
    , m_pRenderContext(pRenderContext)
    , m_pUserData(pUserData)
  {
  }

  /// Resolve a transient texture handle to the real GPU texture.
  ezGALTextureHandle ResolveTexture(ezRenderGraphTextureHandle hTexture) const;

  /// Resolve a transient buffer handle to the real GPU buffer.
  ezGALBufferHandle ResolveBuffer(ezRenderGraphBufferHandle hBuffer) const;

  ezGALCommandEncoder* GetCommandEncoder() const;
  ezGALDevice* GetDevice() const;
  ezRenderContext* GetRenderContext() const;

  /// Gives access to the graph's user data set via `ezRenderGraph::SetUserData`.
  template <typename T>
  const T* GetUserData() const
  {
    return ezDynamicCast<const T*>(m_pUserData);
  }

private:
  friend class ezRenderGraph;

  ezGALCommandEncoder* m_pCommandEncoder = nullptr;
  ezGALDevice* m_pDevice = nullptr;
  ezRenderContext* m_pRenderContext = nullptr;
  const ezReflectedClass* m_pUserData = nullptr;
  const ezDynamicArray<ezUInt16>* m_pTextureToResolvedTexture = nullptr;
  const ezDynamicArray<ezUInt16>* m_pBufferToResolvedBuffer = nullptr;
  const ezDynamicArray<ezGALTextureHandle>* m_pResolvedTextures = nullptr;
  const ezDynamicArray<ezGALBufferHandle>* m_pResolvedBuffers = nullptr;
};

/// Execution callback type for render graph passes.
using ezRenderGraphExecuteFunction = ezDelegate<void(const ezRenderGraphContext&)>;
