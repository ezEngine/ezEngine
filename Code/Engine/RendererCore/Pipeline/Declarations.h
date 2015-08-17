#pragma once

#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>
#include <Core/World/Declarations.h>
#include <RendererCore/Basics.h>

class ezCamera;
class ezView;
class ezRenderPipeline;
class ezRenderPipelinePass;
class ezGALContext;
class ezRenderContext;
struct ezViewData;

struct ezRenderViewContext
{
  const ezCamera* m_pCamera;
  const ezViewData* m_pViewData;
  ezRenderContext* m_pRenderContext;
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
  ezGameObjectHandle m_hOwner;

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
  virtual ezUInt32 Render(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData* const>& renderData) = 0;
};


struct EZ_RENDERERCORE_DLL ezExtractRenderDataMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezExtractRenderDataMessage);

  const ezView* m_pView;
};
