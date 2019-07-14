#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

class QRectF;

namespace ezWidgetUtils
{
  EZ_GUIFOUNDATION_DLL void AdjustGridDensity(double& fFinestDensity, double& fRoughDensity, ezUInt32 uiWindowWidth, double fViewportSceneWidth, ezUInt32 uiMinPixelsForStep);

  EZ_GUIFOUNDATION_DLL void ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX);

  EZ_GUIFOUNDATION_DLL void ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY);
}

