#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Math/Vec3.h>

/// \brief Enum values for success and failure. To be used by functions as return values mostly, instead of bool.
struct ezCallbackResult
{
  enum Enum
  {
    Stop,     ///< The calling function should stop expanding in this direction (might mean it should abort entirely)
    Continue, ///< The calling function should continue further.
  };
};

/// \brief Enum values for the result of some rasterization functions.
struct ezRasterizationResult
{
  enum Enum
  {
    Aborted,  ///< The function was aborted before it reached the end.
    Finished, ///< The function rasterized all possible points.
  };
};

namespace ez2DGridUtils
{
  /// \brief The callback declaration for the function that needs to be passed to ComputePointsOnLine().
  typedef ezCallbackResult::Enum (*EZ_RASTERIZED_POINT_CALLBACK)(ezInt32 x, ezInt32 y, void* pPassThrough);

  /// \brief Computes all the points on a 2D line and calls a function to report every point.
  ///
  /// The function implements Bresenham's algorithm for line rasterization. The first point to be reported through the
  /// callback is always the start position, the last point is always the end position.
  /// pPassThrough is passed through to the user callback for custom data.
  ///
  /// The function returns ezRasterizationResult::Aborted if the callback returned ezCallbackResult::Stop at any time
  /// and the line will not be computed further in that case.
  /// It returns ezRasterizationResult::Finished if the entire line was rasterized.
  ///
  /// This function does not do any dynamic memory allocations internally.
  EZ_GAMEUTILS_DLL ezRasterizationResult::Enum ComputePointsOnLine(ezInt32 iStartX, ezInt32 iStartY, ezInt32 iEndX, ezInt32 iEndY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = NULL);

  /// \brief Computes all the points on a 2D circle and calls a function to report every point.
  ///
  /// The points are reported in a rather chaotic order (ie. when one draws a line from point to point, it does not yield a circle shape).
  /// The callback may abort the operation by returning ezCallbackResult::Stop.
  ///
  /// This function does not do any dynamic memory allocations internally.
  EZ_GAMEUTILS_DLL ezRasterizationResult::Enum ComputePointsOnCircle(ezInt32 iStartX, ezInt32 iStartY, ezUInt32 uiRadius, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = NULL);

  /// \brief Starts at the given point and then fills all surrounding cells until a border is detected.
  ///
  /// The callback should return ezCallbackResult::Continue for each cell that has not been visited so far and for which all four direct
  /// neighbors should be visited. If the floodfill algorithm leaves the valid area, the callback must return ezCallbackResult::Stop to signal
  /// a border. Thus the callback must be able to handle point positions outside the valid range and it also needs to be able to detect
  /// which cells have been visited before, as the FloodFill function will not keep that state internally.
  ///
  /// The function returns the number of cells that were visited and returned ezCallbackResult::Continue (ie. which were not classified as
  /// border cells).
  ///
  /// Note that the FloodFill function requires an internal queue to store which cells still need to be visited, as such it will do
  /// dynamic memory allocations.
  EZ_GAMEUTILS_DLL ezUInt32 FloodFill(ezInt32 iStartX, ezInt32 iStartY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = NULL);
}

