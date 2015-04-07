#pragma once

#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Basics.h>

class ezGameObject;
class ezView;
class ezRenderPipeline;
class ezRenderPipelinePass;
class ezGALContext;
class ezRenderContext;

struct ezRenderViewContext
{
  const ezView* m_pView;
  ezRenderContext* m_pRenderer;
};

class EZ_RENDERERCORE_DLL ezRenderData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderData);

public:

  EZ_FORCE_INLINE ezUInt64 GetSortingKey() const
  {
    return m_uiSortingKey;
  }

private:
  friend class ezRenderPipeline;

  ezUInt64 m_uiSortingKey;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const ezGameObject* m_pOwner; /// debugging only
#endif
};

typedef ezUInt32 ezRenderPassType;

class EZ_RENDERERCORE_DLL ezRenderer : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderer);

public:
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) = 0;

  /// \brief Should return the number of objects which have been rendered
  virtual ezUInt32 Render(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData*>& renderData) = 0;
};


struct ezExtractRenderDataMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezExtractRenderDataMessage);

  ezRenderPipeline* m_pRenderPipeline;
  const ezView* m_pView;
};
