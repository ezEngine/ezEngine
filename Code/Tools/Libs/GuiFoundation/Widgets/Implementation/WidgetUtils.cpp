#include <GuiFoundationPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Declarations.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QApplication>
#include <QRect>

QScreen& ezWidgetUtils::GetClosestScreen(const QPoint& point)
{
  QScreen* pClosestScreen = QApplication::screenAt(point);
  if (pClosestScreen == nullptr)
  {
    QList<QScreen*> screens = QApplication::screens();
    float fShortestDistance = ezMath::Infinity<float>();
    for (QScreen* pScreen : screens)
    {
      const QRect geom = pScreen->geometry();
      ezBoundingBox ezGeom;
      ezGeom.SetCenterAndHalfExtents(ezVec3(geom.center().x(), geom.center().y(), 0), ezVec3(geom.width() / 2.0f, geom.height() / 2.0f, 0));
      const ezVec3 ezPoint(point.x(), point.y(), 0);
      if (ezGeom.Contains(ezPoint))
      {
        return *pScreen;
      }
      float fDistance = ezGeom.GetDistanceSquaredTo(ezPoint);
      if (fDistance < fShortestDistance)
      {
        fShortestDistance = fDistance;
        pClosestScreen = pScreen;
      }
    }
    EZ_ASSERT_DEV(pClosestScreen != nullptr, "There are no screens connected, UI cannot function.");
  }
  return *pClosestScreen;
}

void ezWidgetUtils::AdjustGridDensity(double& fFinestDensity, double& fRoughDensity, ezUInt32 uiWindowWidth, double fViewportSceneWidth,
  ezUInt32 uiMinPixelsForStep)
{
  const double fMaxStepsFitInWindow = (double)uiWindowWidth / (double)uiMinPixelsForStep;

  const double fStartDensity = fFinestDensity;

  ezInt32 iFactor = 1;
  double fNewDensity = fFinestDensity;
  ezInt32 iFactors[2] = {5, 2};
  ezInt32 iLastFactor = 0;

  while (true)
  {
    const double fStepsAtDensity = fViewportSceneWidth / fNewDensity;

    if (fStepsAtDensity < fMaxStepsFitInWindow)
      break;

    iFactor *= iFactors[iLastFactor];
    fNewDensity = fStartDensity * iFactor;

    iLastFactor = (iLastFactor + 1) % 2;
  }

  fFinestDensity = fStartDensity * iFactor;

  iFactor *= iFactors[iLastFactor];
  fRoughDensity = fStartDensity * iFactor;
}

void ezWidgetUtils::ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = ezMath::RoundDown((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = ezMath::RoundUp((double)viewportSceneRect.right(), fGridStops);
}

void ezWidgetUtils::ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY)
{
  out_fMinY = ezMath::RoundDown((double)viewportSceneRect.top(), fGridStops);
  out_fMaxY = ezMath::RoundUp((double)viewportSceneRect.bottom(), fGridStops);
}
