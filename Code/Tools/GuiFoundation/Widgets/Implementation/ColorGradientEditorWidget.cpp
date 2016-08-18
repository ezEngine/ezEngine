#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>

QColorGradientEditorWidget::QColorGradientEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  GradientWidget->setColorGradientData(&m_Gradient);
  GradientWidget->setEditMode(true);
  GradientWidget->FrameExtents();

  GradientWidget->setShowColorCPs(true);
  GradientWidget->setShowAlphaCPs(true);
  GradientWidget->setShowCoords(true, true);

  on_GradientWidget_selectionChanged(-1, -1, -1);

  connect(GradientWidget, &QColorGradientWidget::addColorCp, this, [this](float x, const ezColorGammaUB& color) { emit ColorCpAdded(x, color); });
  connect(GradientWidget, &QColorGradientWidget::moveColorCpToPos, this, [this](ezInt32 idx, float x) { emit ColorCpMoved(idx, x); });
  connect(GradientWidget, &QColorGradientWidget::deleteColorCp, this, [this](ezInt32 idx) { emit ColorCpDeleted(idx); });

  connect(GradientWidget, &QColorGradientWidget::addAlphaCp, this, [this](float x, ezUInt8 alpha) { emit AlphaCpAdded(x, alpha); });
  connect(GradientWidget, &QColorGradientWidget::moveAlphaCpToPos, this, [this](ezInt32 idx, float x) { emit AlphaCpMoved(idx, x); });
  connect(GradientWidget, &QColorGradientWidget::deleteAlphaCp, this, [this](ezInt32 idx) { emit AlphaCpDeleted(idx); });

  connect(GradientWidget, &QColorGradientWidget::addIntensityCp, this, [this](float x, float intensity) { emit IntensityCpAdded(x, intensity); });
  connect(GradientWidget, &QColorGradientWidget::moveIntensityCpToPos, this, [this](ezInt32 idx, float x) { emit IntensityCpMoved(idx, x); });
  connect(GradientWidget, &QColorGradientWidget::deleteIntensityCp, this, [this](ezInt32 idx) { emit IntensityCpDeleted(idx); });

  connect(GradientWidget, &QColorGradientWidget::beginOperation, this, [this]() { emit BeginOperation(); });
  connect(GradientWidget, &QColorGradientWidget::endOperation, this, [this](bool commit) { emit EndOperation(commit); });

  connect(GradientWidget, &QColorGradientWidget::triggerPickColor, this, [this]() { on_ButtonColor_clicked(); });
}


QColorGradientEditorWidget::~QColorGradientEditorWidget()
{
}


void QColorGradientEditorWidget::SetColorGradient(const ezColorGradient& gradient)
{
  bool clearSelection = false;

  // clear selection if the number of control points has changed
  {
    ezUInt32 numRgb = 0xFFFFFFFF, numRgb2 = 0xFFFFFFFF;
    ezUInt32 numAlpha = 0xFFFFFFFF, numAlpha2 = 0xFFFFFFFF;
    ezUInt32 numIntensity = 0xFFFFFFFF, numIntensity2 = 0xFFFFFFFF;

    gradient.GetNumControlPoints(numRgb, numAlpha, numIntensity);
    m_Gradient.GetNumControlPoints(numRgb2, numAlpha2, numIntensity2);

    if (numRgb != numRgb2 || numAlpha != numAlpha2 || numIntensity != numIntensity2)
      clearSelection = true;
  }

  const bool wasEmpty = m_Gradient.IsEmpty();

  m_Gradient = gradient;

  {
    QtScopedUpdatesDisabled ud(this);

    if (wasEmpty)
      GradientWidget->FrameExtents();

    if (clearSelection)
      GradientWidget->ClearSelectedCP();
  }

  UpdateCpUi();

  GradientWidget->update();
}

void QColorGradientEditorWidget::on_ButtonFrame_clicked()
{
  GradientWidget->FrameExtents();
  GradientWidget->update();
}

void QColorGradientEditorWidget::on_GradientWidget_selectionChanged(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP)
{
  m_iSelectedColorCP = colorCP;
  m_iSelectedAlphaCP = alphaCP;
  m_iSelectedIntensityCP = intensityCP;

  SpinPosition->setEnabled((m_iSelectedColorCP != -1) || (m_iSelectedAlphaCP != -1) || (m_iSelectedIntensityCP != -1));

  LabelColor->setVisible(m_iSelectedColorCP != -1);
  ButtonColor->setVisible(m_iSelectedColorCP != -1);

  LabelAlpha->setVisible(m_iSelectedAlphaCP != -1);
  SpinAlpha->setVisible(m_iSelectedAlphaCP != -1);
  SliderAlpha->setVisible(m_iSelectedAlphaCP != -1);

  LabelIntensity->setVisible(m_iSelectedIntensityCP != -1);
  SpinIntensity->setVisible(m_iSelectedIntensityCP != -1);

  UpdateCpUi();
}


void QColorGradientEditorWidget::on_SpinPosition_valueChanged(double value)
{
  if (m_iSelectedColorCP != -1)
  {
    emit ColorCpMoved(m_iSelectedColorCP, value);
  }
  else if (m_iSelectedAlphaCP != -1)
  {
    emit AlphaCpMoved(m_iSelectedAlphaCP, value);
  }
  else if (m_iSelectedIntensityCP != -1)
  {
    emit IntensityCpMoved(m_iSelectedIntensityCP, value);
  }
}


void QColorGradientEditorWidget::on_SpinAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    emit AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}

void QColorGradientEditorWidget::on_SliderAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    emit AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}


void QColorGradientEditorWidget::on_SliderAlpha_sliderPressed()
{
  emit BeginOperation();
}


void QColorGradientEditorWidget::on_SliderAlpha_sliderReleased()
{
  emit EndOperation(true);
}

void QColorGradientEditorWidget::on_SpinIntensity_valueChanged(double value)
{
  if (m_iSelectedIntensityCP != -1)
  {
    emit IntensityCpChanged(m_iSelectedIntensityCP, value);
  }
}


void QColorGradientEditorWidget::on_ButtonColor_clicked()
{
  if (m_iSelectedColorCP != -1)
  {
    const auto& cp = m_Gradient.GetColorControlPoint(m_iSelectedColorCP);

    m_PickColorStart = ezColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
    m_PickColorCurrent = m_PickColorStart;

    emit BeginOperation();

    ezUIServices::GetSingleton()->ShowColorDialog(m_PickColorStart, false, this,
                                                  SLOT(on_CurrentColor_changed(const QColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
  }
}

void QColorGradientEditorWidget::on_CurrentColor_changed(const QColor& col)
{
  if (m_iSelectedColorCP != -1)
  {
    m_PickColorCurrent = ezColorGammaUB(col.red(), col.green(), col.blue());

    emit ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);
  }
}

void QColorGradientEditorWidget::on_Color_accepted()
{
  if (m_iSelectedColorCP != -1)
  {
    emit ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);

    emit EndOperation(true);
  }
}

void QColorGradientEditorWidget::on_Color_reset()
{
  if (m_iSelectedColorCP != -1)
  {
    emit ColorCpChanged(m_iSelectedColorCP, m_PickColorStart);

    emit EndOperation(false);
  }
}

void QColorGradientEditorWidget::UpdateCpUi()
{
  QtScopedBlockSignals bs(this);
  QtScopedUpdatesDisabled ud(this);

  if (m_iSelectedColorCP != -1)
  {
    const auto& cp = m_Gradient.GetColorControlPoint(m_iSelectedColorCP);

    SpinPosition->setValue(cp.m_PosX);

    QColor col;
    col.setRgb(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);

    ButtonColor->setAutoFillBackground(true);
    QPalette pal = ButtonColor->palette();
    pal.setColor(QPalette::Button, col);
    ButtonColor->setPalette(pal);
  }

  if (m_iSelectedAlphaCP != -1)
  {
    SpinPosition->setValue(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_PosX);
    SpinAlpha->setValue(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_Alpha);
    SliderAlpha->setValue(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_Alpha);
  }

  if (m_iSelectedIntensityCP != -1)
  {
    SpinPosition->setValue(m_Gradient.GetIntensityControlPoint(m_iSelectedIntensityCP).m_PosX);
    SpinIntensity->setValue(m_Gradient.GetIntensityControlPoint(m_iSelectedIntensityCP).m_Intensity);
  }
}


