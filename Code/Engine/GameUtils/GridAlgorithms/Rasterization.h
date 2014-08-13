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
  EZ_GAMEUTILS_DLL ezRasterizationResult::Enum ComputePointsOnLine(ezInt32 iStartX, ezInt32 iStartY, ezInt32 iEndX, ezInt32 iEndY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = nullptr);

  /// \brief Computes all the points on a 2D line and calls a function to report every point.
  ///
  /// Contrary to ComputePointsOnLine() this function does not do diagonal steps but inserts horizontal or vertical steps, such that
  /// reported cells are always connected by an edge.
  /// However, since there are always two possibilities to go from one cell to a diagonal cell, this function tries both and as long
  /// as one of them reports ezCallbackResult::Continue, it will continue. Only if both cells are blocked will the algorithm abort.
  ///
  /// If bVisitBothNeighbors is false, the line will continue with the diagonal cell if the first tried neighbor cell is free.
  /// However, if bVisitBothNeighbors is true, the second alternative cell is also reported to the callback, even though its return value
  /// has no effect on whether the line continues or aborts.
  EZ_GAMEUTILS_DLL ezRasterizationResult::Enum ComputePointsOnLineConservative(ezInt32 iStartX, ezInt32 iStartY, ezInt32 iEndX, ezInt32 iEndY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = nullptr, bool bVisitBothNeighbors = false);

  /// \brief Computes all the points on a 2D circle and calls a function to report every point.
  ///
  /// The points are reported in a rather chaotic order (ie. when one draws a line from point to point, it does not yield a circle shape).
  /// The callback may abort the operation by returning ezCallbackResult::Stop.
  ///
  /// This function does not do any dynamic memory allocations internally.
  EZ_GAMEUTILS_DLL ezRasterizationResult::Enum ComputePointsOnCircle(ezInt32 iStartX, ezInt32 iStartY, ezUInt32 uiRadius, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = nullptr);

  /// \brief Starts at the given point and then fills all surrounding cells until a border is detected.
  ///
  /// The callback should return ezCallbackResult::Continue for each cell that has not been visited so far and for which all four direct
  /// neighbors should be visited. If the flood-fill algorithm leaves the valid area, the callback must return ezCallbackResult::Stop to signal
  /// a border. Thus the callback must be able to handle point positions outside the valid range and it also needs to be able to detect
  /// which cells have been visited before, as the FloodFill function will not keep that state internally.
  ///
  /// The function returns the number of cells that were visited and returned ezCallbackResult::Continue (ie. which were not classified as
  /// border cells).
  ///
  /// Note that the FloodFill function requires an internal queue to store which cells still need to be visited, as such it will do
  /// dynamic memory allocations. You can pass in a queue that will be used as the temp buffer, thus you can reuse the same container for
  /// several operations, which will reduce the amount of memory allocations that need to be done.
  EZ_GAMEUTILS_DLL ezUInt32 FloodFill(ezInt32 iStartX, ezInt32 iStartY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = nullptr, ezDeque<ezVec2I32>* pTempArray = nullptr);

  /// \brief Describes the different circle types that can be rasterized
  enum ezBlobType
  {
    Point1x1,     ///< The circle has just one point at the center
    Cross3x3,     ///< The circle has 5 points, one at the center, 1 at each edge of that
    Block3x3,     ///< The 'circle' is just a 3x3 rectangle (9 points)
    Circle5x5,    ///< The circle is a rectangle with each of the 4 corner points missing (21 points)
    Circle7x7,    ///< The circle is a actually starts looking like a circle (37 points)
    Circle9x9,    ///< Circle with 57 points
    Circle11x11,  ///< Circle with 97 points
    Circle13x13,  ///< Circle with 129 points
    Circle15x15,  ///< Circle with 177 points
  };

  /// \brief Rasterizes a circle of limited dimensions and calls the given callback for each point.
  ///
  /// See ezCircleType for the available circle types. Those circles are handcrafted to have good looking shapes at low resolutions.
  /// This type of circle is not meant for actually rendering circles, but for doing area operations and overlapping checks for game
  /// units, visibility determination etc. Basically everything that is usually small, but where a simple point might not suffice.
  /// For example most units in a strategy game might only occupy a single cell, but some units might be larger and thus need to occupy
  /// the surrounding cells as well. Using RasterizeBlob() you can compute the units footprint easily.
  ///
  /// RasterizeBlob() will stop immediately and return ezRasterizationResult::Aborted when the callback function returns ezCallbackResult::Stop.
  EZ_GAMEUTILS_DLL ezRasterizationResult::Enum RasterizeBlob(ezInt32 iPosX, ezInt32 iPosY, ezBlobType eType, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = nullptr);

  /// \brief Rasterizes a circle of any size (unlike RasterizeBlob()), though finding the right radius values for nice looking small circles can be more difficult.
  ///
  /// This function rasterizes a full circle. The radius is a float value, ie. you can use fractional values to shave off cells at the borders
  /// bit by bit.
  ///
  /// RasterizeCircle() will stop immediately and return ezRasterizationResult::Aborted when the callback function returns ezCallbackResult::Stop.
  EZ_GAMEUTILS_DLL ezRasterizationResult::Enum RasterizeCircle(ezInt32 iPosX, ezInt32 iPosY, float fRadius, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = nullptr);


  /// \brief Computes which points are visible from the start position by tracing lines radially outwards.
  ///
  /// The center start position is at iPosX, iPosY and uiRadius defines the maximum distance that an object can see.
  /// uiWidth and uiHeight define the maximum coordinates at which the end of the grid is reached (and thus the line tracing can early out if it reaches those).
  /// For the minimum coordinate (0, 0) is assumed.
  ///
  /// The callback function must return ezCallbackResult::Continue for cells that are not blocking and ezCallbackResult::Stop for cells that block visibility.
  /// 
  /// The algorithm requires internal state and thus needs to do dynamic memory allocations. If you want to reduce the number of allocations, 
  /// you can pass in your own array, that can be reused for many queries.
  EZ_GAMEUTILS_DLL void ComputeVisibleArea(ezInt32 iPosX, ezInt32 iPosY, ezUInt16 uiRadius, ezUInt32 uiWidth, ezUInt32 uiHeight, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = nullptr, ezDynamicArray<ezUInt8>* pTempArray = nullptr);

  /// \brief Computes which points are visible from the start position by tracing lines radially outwards. Limits the computation to a cone.
  ///
  /// This function works exactly like ComputeVisibleArea() but limits the computation to a cone that is defined by vDirection and ConeAngle.
  EZ_GAMEUTILS_DLL void ComputeVisibleAreaInCone(ezInt32 iPosX, ezInt32 iPosY, ezUInt16 uiRadius, const ezVec2& vDirection, ezAngle ConeAngle, ezUInt32 uiWidth, ezUInt32 uiHeight, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough = nullptr, ezDynamicArray<ezUInt8>* pTempArray = nullptr);

}

