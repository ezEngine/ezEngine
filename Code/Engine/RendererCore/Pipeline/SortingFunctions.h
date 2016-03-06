#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class EZ_RENDERERCORE_DLL ezRenderSortingFunctions
{
public:
  static ezUInt64 ByBatchThenFrontToBack(const ezRenderData* pRenderData, const ezCamera& camera);
  static ezUInt64 BackToFrontThenByBatch(const ezRenderData* pRenderData, const ezCamera& camera);
};
