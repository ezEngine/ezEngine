#pragma once

#include <RendererCore/Pipeline/Declarations.h>

/// Provides sorting functions for render data.
///
/// These functions generate 64-bit sorting keys used to order render data for optimal rendering.
/// Different sorting strategies are used for different render passes (opaque vs transparent).
struct EZ_RENDERERCORE_DLL ezRenderSortingFunctions
{
  using StorageType = ezUInt8;

  enum Enum
  {
    ByRenderDataThenFrontToBack,
    BackToFrontThenByRenderData,
    ByDepthOffsetOnly,

    Default = ByRenderDataThenFrontToBack
  };

  using Func = ezUInt64 (*)(const ezRenderData*, const ezCamera&);

  /// \brief Sorts by render data type first, then by render data sorting key, then by depth front-to-back.
  ///
  /// Used for opaque geometry to minimize state changes and benefit from early-z rejection.
  static ezUInt64 ByRenderDataThenFrontToBackFunc(const ezRenderData* pRenderData, const ezCamera& camera);

  /// \brief Sorts by depth back-to-front, then by render data type, then by render data sorting key.
  ///
  /// Used for transparent geometry to ensure correct blending order.
  static ezUInt64 BackToFrontThenByRenderDataFunc(const ezRenderData* pRenderData, const ezCamera& camera);

  /// \brief Sorts only by the render data's depth offset back-to-front, meaning render data with a higher depth offset is rendered first.
  ///
  /// This can be used for special cases like full-screen effects where the render order needs to be fully deterministic.
  static ezUInt64 ByDepthOffsetOnlyFunc(const ezRenderData* pRenderData, const ezCamera& camera);

  /// \brief Returns the sorting function corresponding to the given enum value.
  static Func GetFunction(Enum sortingFunction);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderSortingFunctions);
