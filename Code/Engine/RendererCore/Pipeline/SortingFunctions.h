#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class EZ_RENDERERCORE_DLL ezRenderSortingFunctions
{
public:
  static ezUInt64 ByRenderDataThenFrontToBack(const ezRenderData* pRenderData, const ezCamera& camera);
  static ezUInt64 BackToFrontThenByRenderData(const ezRenderData* pRenderData, const ezCamera& camera);
};
