#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

class QRectF;
class QScreen;

namespace ezWidgetUtils
{
  /// \brief Contrary to QApplication::screenAt() this function will always succeed with a valid cursor positions
  /// and also with out of bounds cursor positions.
  EZ_GUIFOUNDATION_DLL QScreen& GetClosestScreen(const QPoint& point);

  EZ_GUIFOUNDATION_DLL void AdjustGridDensity(double& fFinestDensity, double& fRoughDensity, ezUInt32 uiWindowWidth, double fViewportSceneWidth, ezUInt32 uiMinPixelsForStep);

  EZ_GUIFOUNDATION_DLL void ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX);

  EZ_GUIFOUNDATION_DLL void ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY);
}

