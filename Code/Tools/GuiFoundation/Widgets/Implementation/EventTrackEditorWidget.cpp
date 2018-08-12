#include <PCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QInputDialog>
#include <QMenu>
#include <QPainterPath>

ezQtEventTrackEditorWidget::ezQtEventTrackEditorWidget(QWidget* pParent)
    : QWidget(pParent)
{
  setupUi(this);

  EventTrackEdit->SetGridBarWidget(GridBarWidget);

  connect(EventTrackEdit, &ezQtEventTrackWidget::DeleteControlPointsEvent, this, &ezQtEventTrackEditorWidget::onDeleteControlPoints);
  connect(EventTrackEdit, &ezQtEventTrackWidget::DoubleClickEvent, this, &ezQtEventTrackEditorWidget::onDoubleClick);
  connect(EventTrackEdit, &ezQtEventTrackWidget::MoveControlPointsEvent, this, &ezQtEventTrackEditorWidget::onMoveControlPoints);
  connect(EventTrackEdit, &ezQtEventTrackWidget::BeginOperationEvent, this, &ezQtEventTrackEditorWidget::onBeginOperation);
  connect(EventTrackEdit, &ezQtEventTrackWidget::EndOperationEvent, this, &ezQtEventTrackEditorWidget::onEndOperation);
  // connect(EventTrackEdit, &ezQtEventTrackWidget::ScaleControlPointsEvent, this, &ezQtEventTrackEditorWidget::onScaleControlPoints);
  connect(EventTrackEdit, &ezQtEventTrackWidget::ContextMenuEvent, this, &ezQtEventTrackEditorWidget::onContextMenu);
  connect(EventTrackEdit, &ezQtEventTrackWidget::SelectionChangedEvent, this, &ezQtEventTrackEditorWidget::onSelectionChanged);

  LinePosition->setEnabled(false);

  DetermineAvailableEvents();
}

ezQtEventTrackEditorWidget::~ezQtEventTrackEditorWidget() = default;

void ezQtEventTrackEditorWidget::SetData(const ezEventTrackData& data, double fMinCurveLength)
{
  ezQtScopedUpdatesDisabled ud(this);
  ezQtScopedBlockSignals bs(this);

  m_pData = &data;
  EventTrackEdit->SetData(&data, fMinCurveLength);

  UpdateSpinBoxes();
}

void ezQtEventTrackEditorWidget::SetScrubberPosition(ezUInt64 uiTick)
{
  EventTrackEdit->SetScrubberPosition(uiTick / 4800.0);
}


void ezQtEventTrackEditorWidget::ClearSelection()
{
  EventTrackEdit->ClearSelection();
}

void ezQtEventTrackEditorWidget::FrameCurve()
{
  EventTrackEdit->FrameCurve();
}

void ezQtEventTrackEditorWidget::on_AddEventButton_clicked()
{
  QString name = QInputDialog::getText(this, "Add Type", "Event Type Name:");

  m_EventSet.AddAvailableEvent(name.toUtf8().data());

  if (m_EventSet.IsModified())
  {
    m_EventSet.WriteToDDL(":project/Events.ddl");

    FillEventComboBox(name.toUtf8().data());
  }
}

void ezQtEventTrackEditorWidget::onDeleteControlPoints()
{
  ezHybridArray<ezUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (selection.IsEmpty())
    return;

  EventTrackEdit->ClearSelection();

  emit BeginCpChangesEvent("Delete Events");

  selection.Sort([](ezUInt32 lhs, ezUInt32 rhs) -> bool { return lhs > rhs; });

  // delete sorted from back to front to prevent point indices becoming invalidated
  for (ezUInt32 pt : selection)
  {
    emit CpDeletedEvent(pt);
  }

  emit EndCpChangesEvent();
}

void ezQtEventTrackEditorWidget::onDoubleClick(double scenePosX, double epsilon)
{
  InsertCpAt(scenePosX, ezMath::Abs(epsilon));
}

void ezQtEventTrackEditorWidget::onMoveControlPoints(double x)
{
  m_ControlPointMove += x;

  ezHybridArray<ezUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (selection.IsEmpty())
    return;

  emit BeginCpChangesEvent("Move Events");

  for (const auto& cpSel : selection)
  {
    auto& cp = m_DataCopy.m_ControlPoints[cpSel];

    double newPos = cp.GetTickAsTime() + m_ControlPointMove;
    newPos = ezMath::Max(newPos, 0.0);

    emit CpMovedEvent(cpSel, m_pData->TickFromTime(newPos));
  }

  emit EndCpChangesEvent();
}

// void ezQtEventTrackEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
//{
//  const auto selection = EventTrackEdit->GetSelection();
//
//  if (selection.IsEmpty())
//    return;
//
//  const ezVec2d ref(refPt.x(), refPt.y());
//  const ezVec2d scale(scaleX, scaleY);
//
//  emit BeginCpChangesEvent("Scale Points");
//
//  for (const auto& cpSel : selection)
//  {
//    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
//    ezVec2d newPos = ref + (ezVec2d(cp.GetTickAsTime(), cp.m_fValue) - ref).CompMul(scale);
//    newPos.x = ezMath::Max(newPos.x, 0.0);
//    newPos.y = ezMath::Clamp(newPos.y, -100000.0, +100000.0);
//
//    emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(newPos.x), newPos.y);
//  }
//
//  emit EndCpChangesEvent();
//}

void ezQtEventTrackEditorWidget::onBeginOperation(QString name)
{
  m_ControlPointMove = 0;
  m_DataCopy = *m_pData;

  emit BeginOperationEvent(name);
}

void ezQtEventTrackEditorWidget::onEndOperation(bool commit)
{
  emit EndOperationEvent(commit);
}

void ezQtEventTrackEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
{
  m_contextMenuScenePos = scenePos;

  QMenu m(this);
  m.setDefaultAction(m.addAction("Add Event", this, SLOT(onAddPoint())));

  ezHybridArray<ezUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (!selection.IsEmpty())
  {
    m.addAction("Delete Events", this, SLOT(onDeleteControlPoints()), QKeySequence(Qt::Key_Delete));
  }

  m.addSeparator();

  m.addAction("Frame", this, [this]() { FrameCurve(); }, QKeySequence(Qt::ControlModifier | Qt::Key_F));

  m.exec(pos);
}

void ezQtEventTrackEditorWidget::onAddPoint()
{
  InsertCpAt(m_contextMenuScenePos.x(), 0.0f);
}

void ezQtEventTrackEditorWidget::InsertCpAt(double posX, double epsilon)
{
  int curveIdx = 0, cpIdx = 0;
  posX = ezMath::Max(posX, 0.0);

  emit InsertCpEvent(m_pData->TickFromTime(posX), ComboType->currentText().toUtf8().data());
}

void ezQtEventTrackEditorWidget::onSelectionChanged()
{
  UpdateSpinBoxes();
}

void ezQtEventTrackEditorWidget::UpdateSpinBoxes()
{
  ezHybridArray<ezUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  ezQtScopedBlockSignals _1(LinePosition, SelectedTypeLabel);

  if (selection.IsEmpty())
  {
    LinePosition->setText(QString());
    LinePosition->setEnabled(false);
    SelectedTypeLabel->setText("Event: none");
    return;
  }

  const double fPos = m_pData->m_ControlPoints[selection[0]].GetTickAsTime();

  LinePosition->setEnabled(true);

  ezStringBuilder labelText("Event: ", m_pData->m_ControlPoints[selection[0]].m_sEvent.GetString());

  bool bMultipleTicks = false;
  for (ezUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const ezString& sName = m_pData->m_ControlPoints[selection[i]].m_sEvent.GetString();
    const double fPos2 = m_pData->m_ControlPoints[selection[i]].GetTickAsTime();

    if (!labelText.FindSubString(sName))
    {
      labelText.Append(", ", sName);
    }

    if (fPos2 != fPos)
    {
      bMultipleTicks = true;
      break;
    }
  }

  LinePosition->setText(bMultipleTicks ? QString() : QString::number(fPos, 'f', 2));
  SelectedTypeLabel->setText(labelText.GetData());
}

void ezQtEventTrackEditorWidget::DetermineAvailableEvents()
{
  m_EventSet.ReadFromDDL(":project/Events.ddl");

  FillEventComboBox(nullptr);
}

void ezQtEventTrackEditorWidget::FillEventComboBox(const char* szCurrent)
{
  QString prev = szCurrent;

  if (prev.isEmpty())
    prev = ComboType->currentText();

  ComboType->clear();

  for (const ezString& type : m_EventSet.GetAvailableEvents())
  {
    ComboType->addItem(type.GetData());
  }

  ComboType->setCurrentText(prev);
}

void ezQtEventTrackEditorWidget::on_LinePosition_editingFinished()
{
  QString sValue = LinePosition->text();

  bool ok = false;
  const double value = sValue.toDouble(&ok);
  if (!ok)
    return;

  if (value < 0)
    return;

  ezHybridArray<ezUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);
  if (selection.IsEmpty())
    return;

  emit BeginCpChangesEvent("Set Event Time");

  ezInt64 tick = m_pData->TickFromTime(value);

  for (const auto& cpSel : selection)
  {
    if (m_pData->m_ControlPoints[cpSel].m_iTick != tick)
      emit CpMovedEvent(cpSel, tick);
  }

  emit EndCpChangesEvent();
}
