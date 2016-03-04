#pragma once

#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>
#include <Core/World/Declarations.h>
#include <RendererCore/Basics.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class ezCamera;
class ezExtractor;
class ezView;
class ezRenderData;
class ezBatchedRenderData;
class ezRenderPipeline;
class ezRenderPipelinePass;
class ezGALContext;
class ezRenderContext;
struct ezNodePin;

struct ezViewData;

/// \brief Passed to ezRenderPipelinePass::SetRenderTargets to inform about
/// existing connections on each input / output pin index.
struct ezRenderPipelinePassConnection
{
  ezRenderPipelinePassConnection()
  {
    m_pOutput = nullptr;
  }

  ezGALTextureCreationDescription m_Desc;
  ezGALTextureHandle m_TextureHandle;
  const ezNodePin* m_pOutput; ///< The output pin that this connection spawns from.
  ezHybridArray<const ezNodePin*, 4> m_Inputs; ///< The various input pins this connection is connected to.
};

struct ezRenderViewContext
{
  const ezCamera* m_pCamera;
  const ezViewData* m_pViewData;
  ezRenderContext* m_pRenderContext;
};

class EZ_RENDERERCORE_DLL ezRenderer : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderer, ezReflectedClass);

public:
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) = 0;

  virtual void RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData* const>& batch) = 0;
};
