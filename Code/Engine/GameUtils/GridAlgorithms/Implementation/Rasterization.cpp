#include <GameUtils/PCH.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>

ezRasterizationResult::Enum ez2DGridUtils::ComputePointsOnLine(ezInt32 iStartX, ezInt32 iStartY, ezInt32 iEndX, ezInt32 iEndY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = NULL */)
{
  // Implements Bresenham's line algorithm:
  // http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

  ezInt32 dx = ezMath::Abs(iEndX - iStartX);
  ezInt32 dy = ezMath::Abs(iEndY - iStartY);

  ezInt32 sx = (iStartX < iEndX) ? 1 : -1;
  ezInt32 sy = (iStartY < iEndY) ? 1 : -1;

  ezInt32 err = dx - dy;

  while (true)
  {
    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (Callback(iStartX, iStartY, pPassThrough) == ezCallbackResult::Stop)
      return ezRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return ezRasterizationResult::Finished;

    ezInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err = err + dx;
      iStartY = iStartY + sy;
    }
  }
}

ezRasterizationResult::Enum ez2DGridUtils::ComputePointsOnCircle(ezInt32 iStartX, ezInt32 iStartY, ezUInt32 uiRadius, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = NULL */)
{
  int f = 1 - uiRadius;
  int ddF_x = 1;
  int ddF_y = -2 * uiRadius;
  int x = 0;
  int y = uiRadius;

  // report the four extremes
  if (Callback(iStartX, iStartY + uiRadius, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
  if (Callback(iStartX, iStartY - uiRadius, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
  if (Callback(iStartX + uiRadius, iStartY, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
  if (Callback(iStartX - uiRadius, iStartY, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;

  // the loop iterates over an eighth of the circle (a 45 degree segment) and then mirrors each point 8 times to fill the entire circle
  while (x < y)
  {
    if(f >= 0) 
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    if (Callback(iStartX + x, iStartY + y, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
    if (Callback(iStartX - x, iStartY + y, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
    if (Callback(iStartX + x, iStartY - y, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
    if (Callback(iStartX - x, iStartY - y, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
    if (Callback(iStartX + y, iStartY + x, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
    if (Callback(iStartX - y, iStartY + x, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
    if (Callback(iStartX + y, iStartY - x, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
    if (Callback(iStartX - y, iStartY - x, pPassThrough) == ezCallbackResult::Stop) return ezRasterizationResult::Aborted;
  }

  return ezRasterizationResult::Finished;
}

ezUInt32 ez2DGridUtils::FloodFill(ezInt32 iStartX, ezInt32 iStartY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = NULL */)
{
  ezUInt32 uiFilled = 0;

  ezDeque<ezVec2I32> Queue;
  Queue.PushBack(ezVec2I32(iStartX, iStartY));

  while (!Queue.IsEmpty())
  {
    ezVec2I32 v = Queue.PeekBack();
    Queue.PopBack();

    if (Callback(v.x, v.y, pPassThrough) == ezCallbackResult::Continue)
    {
      ++uiFilled;

      // put the four neighbors into the queue
      Queue.PushBack(ezVec2I32(v.x - 1, v.y));
      Queue.PushBack(ezVec2I32(v.x + 1, v.y));
      Queue.PushBack(ezVec2I32(v.x, v.y - 1));
      Queue.PushBack(ezVec2I32(v.x, v.y + 1));
    }
  }

  return uiFilled;
}