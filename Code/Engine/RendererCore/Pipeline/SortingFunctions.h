#pragma once

#include <RendererCore/Pipeline/Declarations.h>

/// Provides sorting functions for render data.
///
/// These functions generate 64-bit sorting keys used to order render data for optimal rendering.
/// Different sorting strategies are used for different render passes (opaque vs transparent).
class EZ_RENDERERCORE_DLL ezRenderSortingFunctions
{
public:
  /// \brief Sorts by render data type first, then by render data sorting key, then by depth front-to-back.
  ///
  /// Used for opaque geometry to minimize state changes and benefit from early-z rejection.
  static ezUInt64 ByRenderDataThenFrontToBack(const ezRenderData* pRenderData, const ezCamera& camera);

  /// \brief Sorts by depth back-to-front, then by render data type, then by render data sorting key.
  ///
  /// Used for transparent geometry to ensure correct blending order.
  static ezUInt64 BackToFrontThenByRenderData(const ezRenderData* pRenderData, const ezCamera& camera);

  /// \brief Sorts only by the render data's depth offset and ignores position, type and sorting key.
  ///
  /// This can be used for special cases like full-screen effects where the render order needs to be fully deterministic.
  static ezUInt64 ByDepthOffsetOnly(const ezRenderData* pRenderData, const ezCamera& camera);
};
