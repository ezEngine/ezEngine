#pragma once

#include <RendererCore/Pipeline/Declarations.h>

/// Provides sorting functions for render data.
///
/// These functions generate 64-bit sorting keys used to order render data for optimal rendering.
/// Different sorting strategies are used for different render passes (opaque vs transparent).
class EZ_RENDERERCORE_DLL ezRenderSortingFunctions
{
public:
  /// Sorts by render data type first, then by depth front-to-back.
  ///
  /// Used for opaque geometry to minimize state changes and benefit from early-z rejection.
  static ezUInt64 ByRenderDataThenFrontToBack(const ezRenderData* pRenderData, const ezCamera& camera);

  /// Sorts by depth back-to-front, then by render data type.
  ///
  /// Used for transparent geometry to ensure correct blending order.
  static ezUInt64 BackToFrontThenByRenderData(const ezRenderData* pRenderData, const ezCamera& camera);
};
