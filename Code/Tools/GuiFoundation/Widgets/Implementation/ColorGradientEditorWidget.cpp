#include <PCH.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

ezQtColorGradientEditorWidget::ezQtColorGradientEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  GradientWidget->setColorGradientData(&m_Gradient);
  GradientWidget->setEditMode(true);
  GradientWidget->FrameExtents();

  GradientWidget->setShowColorCPs(true);
  GradientWidget->setShowAlphaCPs(true);
  GradientWidget->setShowIntensityCPs(true);
  GradientWidget->setShowCoords(true, true);

  on_GradientWidget_selectionChanged(-1, -1, -1);

  connect(GradientWidget, &ezQtColorGradientWidget::addColorCp, this, [this](double x, const ezColorGammaUB& color) { emit ColorCpAdded(x, color); });
  connect(GradientWidget, &ezQtColorGradientWidget::moveColorCpToPos, this, [this](ezInt32 idx, double x) { emit ColorCpMoved(idx, x); });
  connect(GradientWidget, &ezQtColorGradientWidget::deleteColorCp, this, [this](ezInt32 idx) { emit ColorCpDeleted(idx); });

  connect(GradientWidget, &ezQtColorGradientWidget::addAlphaCp, this, [this](double x, ezUInt8 alpha) { emit AlphaCpAdded(x, alpha); });
  connect(GradientWidget, &ezQtColorGradientWidget::moveAlphaCpToPos, this, [this](ezInt32 idx, double x) { emit AlphaCpMoved(idx, x); });
  connect(GradientWidget, &ezQtColorGradientWidget::deleteAlphaCp, this, [this](ezInt32 idx) { emit AlphaCpDeleted(idx); });

  connect(GradientWidget, &ezQtColorGradientWidget::addIntensityCp, this, [this](double x, float intensity) { emit IntensityCpAdded(x, intensity); });
  connect(GradientWidget, &ezQtColorGradientWidget::moveIntensityCpToPos, this, [this](ezInt32 idx, double x) { emit IntensityCpMoved(idx, x); });
  connect(GradientWidget, &ezQtColorGradientWidget::deleteIntensityCp, this, [this](ezInt32 idx) { emit IntensityCpDeleted(idx); });

  connect(GradientWidget, &ezQtColorGradientWidget::beginOperation, this, [this]() { emit BeginOperation(); });
  connect(GradientWidget, &ezQtColorGradientWidget::endOperation, this, [this](bool commit) { emit EndOperation(commit); });

  connect(GradientWidget, &ezQtColorGradientWidget::triggerPickColor, this, [this]() { on_ButtonColor_clicked(); });
}


ezQtColorGradientEditorWidget::~ezQtColorGradientEditorWidget()
{
}


void ezQtColorGradientEditorWidget::SetColorGradient(const ezColorGradient& gradient)
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

  //const bool wasEmpty = m_Gradient.IsEmpty();

  m_Gradient = gradient;

  {
    ezQtScopedUpdatesDisabled ud(this);

    //if (wasEmpty)
    //  GradientWidget->FrameExtents();

    if (clearSelection)
      GradientWidget->ClearSelectedCP();
  }

  UpdateCpUi();

  GradientWidget->update();
}

void ezQtColorGradientEditorWidget::SetScrubberPosition(ezUInt64 uiTick)
{
  GradientWidget->SetScrubberPosition(uiTick / 4800.0);
}

void ezQtColorGradientEditorWidget::FrameGradient()
{
  GradientWidget->FrameExtents();
  GradientWidget->update();
}

void ezQtColorGradientEditorWidget::on_ButtonFrame_clicked()
{
  FrameGradient();
}

void ezQtColorGradientEditorWidget::on_GradientWidget_selectionChanged(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP)
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


void ezQtColorGradientEditorWidget::on_SpinPosition_valueChanged(double value)
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


void ezQtColorGradientEditorWidget::on_SpinAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    emit AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}

void ezQtColorGradientEditorWidget::on_SliderAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    emit AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}


void ezQtColorGradientEditorWidget::on_SliderAlpha_sliderPressed()
{
  emit BeginOperation();
}


void ezQtColorGradientEditorWidget::on_SliderAlpha_sliderReleased()
{
  emit EndOperation(true);
}

void ezQtColorGradientEditorWidget::on_SpinIntensity_valueChanged(double value)
{
  if (m_iSelectedIntensityCP != -1)
  {
    emit IntensityCpChanged(m_iSelectedIntensityCP, value);
  }
}


void ezQtColorGradientEditorWidget::on_ButtonColor_clicked()
{
  if (m_iSelectedColorCP != -1)
  {
    const auto& cp = m_Gradient.GetColorControlPoint(m_iSelectedColorCP);

    m_PickColorStart = ezColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
    m_PickColorCurrent = m_PickColorStart;

    emit BeginOperation();

    ezQtUiServices::GetSingleton()->ShowColorDialog(m_PickColorStart, false, false, this, SLOT(onCurrentColorChanged(const ezColor&)), SLOT(onColorAccepted()), SLOT(onColorReset()));
  }
}

void ezQtColorGradientEditorWidget::onCurrentColorChanged(const ezColor& col)
{
  if (m_iSelectedColorCP != -1)
  {
    m_PickColorCurrent = col;

    emit ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);
  }
}

void ezQtColorGradientEditorWidget::onColorAccepted()
{
  if (m_iSelectedColorCP != -1)
  {
    emit ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);

    emit EndOperation(true);
  }
}

void ezQtColorGradientEditorWidget::onColorReset()
{
  if (m_iSelectedColorCP != -1)
  {
    emit ColorCpChanged(m_iSelectedColorCP, m_PickColorStart);

    emit EndOperation(false);
  }
}


void ezQtColorGradientEditorWidget::on_ButtonNormalize_clicked()
{
  emit NormalizeRange();
}

void ezQtColorGradientEditorWidget::UpdateCpUi()
{
  ezQtScopedBlockSignals bs(this);
  ezQtScopedUpdatesDisabled ud(this);

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


