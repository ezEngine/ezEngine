#include <GameUtils/PCH.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>

ezRasterizationResult::Enum ez2DGridUtils::ComputePointsOnLine(ezInt32 iStartX, ezInt32 iStartY, ezInt32 iEndX, ezInt32 iEndY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */)
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

ezRasterizationResult::Enum ez2DGridUtils::ComputePointsOnLineConservative(ezInt32 iStartX, ezInt32 iStartY, ezInt32 iEndX, ezInt32 iEndY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */, bool bVisitBothNeighbors /* = false */)
{
  ezInt32 dx = ezMath::Abs(iEndX - iStartX);
  ezInt32 dy = ezMath::Abs(iEndY - iStartY);

  ezInt32 sx = (iStartX < iEndX) ? 1 : -1;
  ezInt32 sy = (iStartY < iEndY) ? 1 : -1;

  ezInt32 err = dx - dy;

  ezInt32 iLastX = iStartX;
  ezInt32 iLastY = iStartY;

  while (true)
  {
    // if this is going to be a diagonal step, make sure to insert horizontal/vertical steps

    if ((ezMath::Abs(iLastX - iStartX) + ezMath::Abs(iLastY - iStartY)) == 2)
    {
      // This part is the difference to the non-conservative line algorithm

      if (Callback(iLastX, iStartY, pPassThrough) == ezCallbackResult::Continue)
      {
        // first one succeeded, going to continue

        // if this is true, the user still wants a callback for the alternative, even though it does not change the outcome anymore
        if (bVisitBothNeighbors)
          Callback(iStartX, iLastY, pPassThrough);
      }
      else
      {
        // first one failed, try the second
        if (Callback(iStartX, iLastY, pPassThrough) == ezCallbackResult::Stop)
          return ezRasterizationResult::Aborted;
      }
    }

    iLastX = iStartX;
    iLastY = iStartY;

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


ezRasterizationResult::Enum ez2DGridUtils::ComputePointsOnCircle(ezInt32 iStartX, ezInt32 iStartY, ezUInt32 uiRadius, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */)
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
    if (f >= 0) 
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

ezUInt32 ez2DGridUtils::FloodFill(ezInt32 iStartX, ezInt32 iStartY, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */, ezDeque<ezVec2I32>* pTempArray /* = nullptr */)
{
  ezUInt32 uiFilled = 0;

  ezDeque<ezVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(ezVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    ezVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (Callback(v.x, v.y, pPassThrough) == ezCallbackResult::Continue)
    {
      ++uiFilled;

      // put the four neighbors into the queue
      pTempArray->PushBack(ezVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(ezVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(ezVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(ezVec2I32(v.x, v.y + 1));
    }
  }

  return uiFilled;
}


// Lookup table that describes the shape of the circle
// When rasterizing circles with few pixels algorithms usually don't give nice shapes
// so this lookup table is handcrafted for better results
static const ezUInt8 OverlapCircle[15][15] =
{
  {9,9,9,9,9,8,8,8,8,8,9,9,9,9,9},
  {9,9,9,8,8,7,7,7,7,7,8,8,9,9,9},
  {9,9,8,8,7,6,6,6,6,6,7,8,8,9,9},
  {9,8,8,7,6,6,5,5,5,6,6,7,8,8,9},
  {9,8,7,6,6,5,4,4,4,5,6,6,7,8,9},
  {8,7,6,6,5,4,3,3,3,4,5,6,6,7,8},
  {8,7,6,5,4,3,2,1,2,3,4,5,6,7,8},
  {8,7,6,5,4,3,1,0,1,3,4,5,6,7,8},
  {8,7,6,5,4,3,2,1,2,3,4,5,6,7,8},
  {8,7,6,6,5,4,3,3,3,4,5,6,6,7,8},
  {9,8,7,6,6,5,4,4,4,5,6,6,7,8,9},
  {9,8,8,7,6,6,5,5,5,6,6,7,8,8,9},
  {9,9,8,8,7,6,6,6,6,6,7,8,8,9,9},
  {9,9,9,8,8,7,7,7,7,7,8,8,9,9,9},
  {9,9,9,9,9,8,8,8,8,8,9,9,9,9,9}
};

static const ezInt32 CircleCenter = 7;
static const ezUInt8 CircleAreaMin[9] = { 7, 6, 6, 5, 4, 3, 2, 1, 0 };
static const ezUInt8 CircleAreaMax[9] = { 7, 8, 8, 9,10,11,12,13,14 };

ezRasterizationResult::Enum ez2DGridUtils::RasterizeBlob(ezInt32 iPosX, ezInt32 iPosY, ezBlobType eType, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */)
{
  const ezUInt8 uiCircleType = ezMath::Clamp<ezUInt8>(eType, 0, 8);

  const ezInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const ezInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (ezInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (ezInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      if (OverlapCircle[y][x] <= uiCircleType)
      {
        if (Callback(iPosX + x, iPosY + y, pPassThrough) == ezCallbackResult::Stop)
          return ezRasterizationResult::Aborted;
      }
    }
  }

  return ezRasterizationResult::Finished;
}

ezRasterizationResult::Enum ez2DGridUtils::RasterizeCircle(ezInt32 iPosX, ezInt32 iPosY, float fRadius, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */)
{
  const ezVec2 vCenter ((float) iPosX, (float) iPosY);

  const ezInt32 iRadius = (ezInt32) fRadius;
  const float fRadiusSqr = ezMath::Square(fRadius);

  for (ezInt32 y = iPosY - iRadius; y <= iPosY + iRadius; ++y)
  {
    for (ezInt32 x = iPosX - iRadius; x <= iPosX + iRadius; ++x)
    {
      const ezVec2 v((float) x, (float) y);

      if ((v - vCenter).GetLengthSquared() > fRadiusSqr)
        continue;

      if (Callback(x, y, pPassThrough) == ezCallbackResult::Stop)
        return ezRasterizationResult::Aborted;
    }
  }

  return ezRasterizationResult::Finished;
}


struct VisibilityLine
{
  ezDynamicArray<ezUInt8>* m_pVisible;
  ezUInt32 m_uiSize;
  ezUInt32 m_uiRadius;
  ezInt32 m_iCenterX;
  ezInt32 m_iCenterY;
  ez2DGridUtils::EZ_RASTERIZED_POINT_CALLBACK m_VisCallback;
  void* m_pUserPassThrough;
  ezUInt32 m_uiWidth;
  ezUInt32 m_uiHeight;
  ezVec2 m_vDirection;
  ezAngle m_ConeAngle;
};

struct CellFlags
{
  enum Enum
  {
    NotVisited = 0,
    Visited = EZ_BIT(0),
    Visible = Visited | EZ_BIT(1),
    Invisible = Visited,
  };
};

static ezCallbackResult::Enum MarkPointsOnLineVisible(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  VisibilityLine* VisLine = (VisibilityLine*) pPassThrough;

  // if the reported point is outside the playing field, don't continue
  if (x < 0 || y < 0 || x >= (ezInt32) VisLine->m_uiWidth || y >= (ezInt32) VisLine->m_uiHeight)
    return ezCallbackResult::Stop;

  // compute the point position inside our virtual grid (where the start position is at the center)
  const ezUInt32 VisX = x - VisLine->m_iCenterX + VisLine->m_uiRadius;
  const ezUInt32 VisY = y - VisLine->m_iCenterY + VisLine->m_uiRadius;

  // if we are outside our virtual grid, stop 
  if (VisX >= (ezInt32) VisLine->m_uiSize || VisY >= (ezInt32) VisLine->m_uiSize)
    return ezCallbackResult::Stop;

  // We actually only need two bits for each cell (visited + visible)
  // so we pack the information for four cells into one byte
  const ezUInt32 uiCellIndex = VisY * VisLine->m_uiSize + VisX;
  const ezUInt32 uiBitfieldByte = uiCellIndex >> 2; // division by four
  const ezUInt32 uiBitfieldBiteOff = uiBitfieldByte << 2; // modulo to determine where in the byte this cell is stored
  const ezUInt32 uiMaskShift = (uiCellIndex - uiBitfieldBiteOff) * 2; // times two because we use two bits

  ezUInt8& CellFlagsRef = (*VisLine->m_pVisible)[uiBitfieldByte]; // for writing into the byte later
  const ezUInt8 ThisCellsFlags = (CellFlagsRef >> uiMaskShift) & 3U; // the decoded flags value for reading (3U == lower two bits)

  // if this point on the line was already visited and determined to be invisible, don't continue
  if (ThisCellsFlags == CellFlags::Invisible)
    return ezCallbackResult::Stop;

  // this point has been visited already and the point was determined to be visible, so just continue
  if (ThisCellsFlags == CellFlags::Visible)
    return ezCallbackResult::Continue;

  // apparently this cell has not been visited yet, so ask the user callback what to do
  if (VisLine->m_VisCallback(x, y, VisLine->m_pUserPassThrough) == ezCallbackResult::Continue)
  {
    // the callback reported this cell as visible, so flag it and continue
    CellFlagsRef |= ((ezUInt8) CellFlags::Visible) << uiMaskShift;
    return ezCallbackResult::Continue;
  }

  // the callback reported this flag as invisible, flag it and stop the line
  CellFlagsRef |= ((ezUInt8) CellFlags::Invisible) << uiMaskShift;
  return ezCallbackResult::Stop;
}

static ezCallbackResult::Enum MarkPointsInCircleVisible(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*) pPassThrough;

  ez2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return ezCallbackResult::Continue;
}

void ez2DGridUtils::ComputeVisibleArea(ezInt32 iPosX, ezInt32 iPosY, ezUInt16 uiRadius, ezUInt32 uiWidth, ezUInt32 uiHeight, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */, ezDynamicArray<ezUInt8>* pTempArray /* = nullptr */ )
{
  const ezUInt32 uiSize = uiRadius * 2 + 1;

  ezDynamicArray<ezUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(ezMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte

  VisibilityLine ld;
  ld.m_uiSize = uiSize;
  ld.m_uiRadius = uiRadius;
  ld.m_pVisible = pTempArray;
  ld.m_iCenterX = iPosX;
  ld.m_iCenterY = iPosY;
  ld.m_VisCallback = Callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth = uiWidth;
  ld.m_uiHeight = uiHeight;

  // from the center, trace lines to all points on the circle around it
  // each line determines for each cell whether it is visible
  // once an invisible cell is encountered, a line will stop further tracing
  // no cell is ever reported twice to the user callback
  ez2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInCircleVisible, &ld);
}

static ezCallbackResult::Enum MarkPointsInConeVisible(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*) pPassThrough;

  const ezVec2 vPos((float) x, (float) y);
  const ezVec2 vDirToPos = (vPos - ezVec2((float) ld->m_iCenterX, (float) ld->m_iCenterY)).GetNormalized();

  const ezAngle angle = ezMath::ACos(vDirToPos.Dot(ld->m_vDirection));

  if (angle.GetRadian() < ld->m_ConeAngle.GetRadian())
    ez2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return ezCallbackResult::Continue;
}

void ez2DGridUtils::ComputeVisibleAreaInCone(ezInt32 iPosX, ezInt32 iPosY, ezUInt16 uiRadius, const ezVec2& vDirection, ezAngle ConeAngle, ezUInt32 uiWidth, ezUInt32 uiHeight, EZ_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */, ezDynamicArray<ezUInt8>* pTempArray /* = nullptr */)
{
  const ezUInt32 uiSize = uiRadius * 2 + 1;

  ezDynamicArray<ezUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(ezMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte


  VisibilityLine ld;
  ld.m_uiSize = uiSize;
  ld.m_uiRadius = uiRadius;
  ld.m_pVisible = pTempArray;
  ld.m_iCenterX = iPosX;
  ld.m_iCenterY = iPosY;
  ld.m_VisCallback = Callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth = uiWidth;
  ld.m_uiHeight = uiHeight;
  ld.m_vDirection = vDirection;
  ld.m_ConeAngle = ConeAngle;

  ez2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInConeVisible, &ld);
}



EZ_STATICLINK_FILE(GameUtils, GameUtils_GridAlgorithms_Implementation_Rasterization);

