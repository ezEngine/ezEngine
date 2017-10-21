#include <PCH.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Math/Color8UNorm.h>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QGraphicsSceneEvent>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>

ezQtCurve1DEditorWidget::ezQtCurve1DEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  CurveEdit->SetGridBarWidget(GridBarWidget);

  connect(CurveEdit, &ezQtCurveEditWidget::DeleteControlPointsEvent, this, &ezQtCurve1DEditorWidget::onDeleteControlPoints);
  connect(CurveEdit, &ezQtCurveEditWidget::DoubleClickEvent, this, &ezQtCurve1DEditorWidget::onDoubleClick);
  connect(CurveEdit, &ezQtCurveEditWidget::MoveControlPointsEvent, this, &ezQtCurve1DEditorWidget::onMoveControlPoints);
  connect(CurveEdit, &ezQtCurveEditWidget::MoveTangentsEvent, this, &ezQtCurve1DEditorWidget::onMoveTangents);
  connect(CurveEdit, &ezQtCurveEditWidget::BeginOperation, this, &ezQtCurve1DEditorWidget::onBeginOperation);
  connect(CurveEdit, &ezQtCurveEditWidget::EndOperation, this, [this](bool commit) { emit EndOperation(commit); });
  connect(CurveEdit, &ezQtCurveEditWidget::ScaleControlPointsEvent, this, &ezQtCurve1DEditorWidget::onScaleControlPoints);
}

ezQtCurve1DEditorWidget::~ezQtCurve1DEditorWidget()
{
}

void ezQtCurve1DEditorWidget::SetCurves(const ezArrayPtr<ezCurve1D>& curves)
{
  ezQtScopedUpdatesDisabled ud(this);
  ezQtScopedBlockSignals bs(this);

  m_Curves = curves;

  CurveEdit->SetCurves(curves);
}

void ezQtCurve1DEditorWidget::FrameCurve()
{

  //curveWidget->update();
}

void ezQtCurve1DEditorWidget::on_ButtonFrame_clicked()
{
  FrameCurve();
}

//void ezQtCurve1DEditorWidget::on_curveWidget_selectionChanged(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP)
//{
//  m_iSelectedColorCP = colorCP;
//  m_iSelectedAlphaCP = alphaCP;
//  m_iSelectedIntensityCP = intensityCP;
//
//  SpinPosition->setEnabled((m_iSelectedColorCP != -1) || (m_iSelectedAlphaCP != -1) || (m_iSelectedIntensityCP != -1));
//
//  LabelColor->setVisible(m_iSelectedColorCP != -1);
//  ButtonColor->setVisible(m_iSelectedColorCP != -1);
//
//  LabelAlpha->setVisible(m_iSelectedAlphaCP != -1);
//  SpinAlpha->setVisible(m_iSelectedAlphaCP != -1);
//  SliderAlpha->setVisible(m_iSelectedAlphaCP != -1);
//
//  LabelIntensity->setVisible(m_iSelectedIntensityCP != -1);
//  SpinIntensity->setVisible(m_iSelectedIntensityCP != -1);
//
//  UpdateCpUi();
//}


void ezQtCurve1DEditorWidget::on_SpinPosition_valueChanged(double value)
{
  //if (m_iSelectedColorCP != -1)
  //{
  //  emit ColorCpMoved(m_iSelectedColorCP, value);
  //}
}

void ezQtCurve1DEditorWidget::on_ButtonNormalizeX_clicked()
{
  emit NormalizeRangeX();
}


void ezQtCurve1DEditorWidget::on_ButtonNormalizeY_clicked()
{
  emit NormalizeRangeY();
}

struct PtToDelete
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiCurveIdx;
  ezUInt32 m_uiPointIdx;

  bool operator<(const PtToDelete& rhs) const
  {
    return m_uiPointIdx > rhs.m_uiPointIdx;
  }
};

void ezQtCurve1DEditorWidget::onDeleteControlPoints()
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  CurveEdit->ClearSelection();

  emit BeginCpChanges("Delete Points");

  ezHybridArray<PtToDelete, 16> delOrder;

  for (const auto& item : selection)
  {
    auto& pt = delOrder.ExpandAndGetRef();
    pt.m_uiCurveIdx = item.m_uiCurve;
    pt.m_uiPointIdx = item.m_uiPoint;
  }

  delOrder.Sort();

  // delete sorted from back to front to prevent point indices becoming invalidated
  for (const auto& pt : delOrder)
  {
    emit CpDeleted(pt.m_uiCurveIdx, pt.m_uiPointIdx);
  }

  emit EndCpChanges();
}

void ezQtCurve1DEditorWidget::onDoubleClick(const QPointF& scenePos, const QPointF& epsilon)
{
  InsertCpAt(scenePos.x(), scenePos.y(), epsilon.x());
}

void ezQtCurve1DEditorWidget::onMoveControlPoints(double x, double y)
{
  m_ControlPointMove += ezVec2(x, y);

  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  emit BeginCpChanges("Move Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_CurvesBackup[cpSel.m_uiCurve].GetControlPoint(cpSel.m_uiPoint);
    const ezVec2 newPos = cp.m_Position + m_ControlPointMove;

    emit CpMoved(cpSel.m_uiCurve, cpSel.m_uiPoint, newPos.x, newPos.y);
  }

  emit EndCpChanges();
}

void ezQtCurve1DEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  const ezVec2 ref(refPt.x(), refPt.y());
  const ezVec2 scale(scaleX, scaleY);

  emit BeginCpChanges("Scale Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_CurvesBackup[cpSel.m_uiCurve].GetControlPoint(cpSel.m_uiPoint);
    const ezVec2 newPos = ref + (cp.m_Position - ref).CompMul(scale);

    emit CpMoved(cpSel.m_uiCurve, cpSel.m_uiPoint, newPos.x, newPos.y);
  }

  emit EndCpChanges();
}

void ezQtCurve1DEditorWidget::onMoveTangents(double x, double y)
{
  m_TangentMove += ezVec2(x, y);

  ezInt32 iCurve;
  ezInt32 iPoint;
  bool bLeftTangent;

  if (!CurveEdit->GetSelectedTangent(iCurve, iPoint, bLeftTangent))
    return;

  emit BeginCpChanges("Move Tangents");

  {
    const auto& cp = m_CurvesBackup[iCurve].GetControlPoint(iPoint);
    ezVec2 newPos;

    if (bLeftTangent)
      newPos = cp.m_LeftTangent + m_TangentMove;
    else
      newPos = cp.m_RightTangent + m_TangentMove;

    emit TangentMoved(iCurve, iPoint, newPos.x, newPos.y, !bLeftTangent);
  }

  emit EndCpChanges();
}

void ezQtCurve1DEditorWidget::onBeginOperation(QString name)
{
  m_CurvesBackup = m_Curves;
  m_TangentMove.SetZero();
  m_ControlPointMove.SetZero();

  emit BeginOperation(name);
}

