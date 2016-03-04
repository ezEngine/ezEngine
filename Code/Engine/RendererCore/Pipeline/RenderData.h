#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Foundation/Memory/FrameAllocator.h>

class EZ_RENDERERCORE_DLL ezRenderData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderData, ezReflectedClass);

public:
  typedef ezUInt32 Category;

  static Category FindOrRegisterCategory(const char* szCategoryName);

  static const char* GetCategoryName(Category category);
  static ezProfilingId& GetCategoryProfilingID(Category category);

  ezUInt32 m_uiBatchId;
  ezGameObjectHandle m_hOwner;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const ezGameObject* m_pOwner; /// debugging only
#endif
};

template <typename T>
static T* CreateRenderDataForThisFrame(const ezGameObject* pOwner, ezUInt32 uiBatchId)
{
  EZ_CHECK_AT_COMPILETIME(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

  T* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), T);
  pRenderData->m_uiBatchId = uiBatchId;
  pRenderData->m_hOwner = pOwner->GetHandle();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}

struct EZ_RENDERERCORE_DLL ezDefaultRenderDataCategories
{
  static ezRenderData::Category Light;
  static ezRenderData::Category Opaque;
  static ezRenderData::Category Masked;
  static ezRenderData::Category Transparent;
  static ezRenderData::Category Foreground1;
  static ezRenderData::Category Foreground2;
  static ezRenderData::Category Selection;
};

struct EZ_RENDERERCORE_DLL ezExtractRenderDataMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezExtractRenderDataMessage);

  const ezView* m_pView;
  ezBatchedRenderData* m_pBatchedRenderData;
  ezRenderData::Category m_OverrideCategory;
};

