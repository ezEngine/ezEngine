#include <PCH.h>

#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>

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

  connect(GradientWidget, &ezQtColorGradientWidget::addColorCp, this,
          [this](double x, const ezColorGammaUB& color) { Q_EMIT ColorCpAdded(x, color); });
  connect(GradientWidget, &ezQtColorGradientWidget::moveColorCpToPos, this, [this](ezInt32 idx, double x) { Q_EMIT ColorCpMoved(idx, x); });
  connect(GradientWidget, &ezQtColorGradientWidget::deleteColorCp, this, [this](ezInt32 idx) { Q_EMIT ColorCpDeleted(idx); });

  connect(GradientWidget, &ezQtColorGradientWidget::addAlphaCp, this, [this](double x, ezUInt8 alpha) { Q_EMIT AlphaCpAdded(x, alpha); });
  connect(GradientWidget, &ezQtColorGradientWidget::moveAlphaCpToPos, this, [this](ezInt32 idx, double x) { Q_EMIT AlphaCpMoved(idx, x); });
  connect(GradientWidget, &ezQtColorGradientWidget::deleteAlphaCp, this, [this](ezInt32 idx) { Q_EMIT AlphaCpDeleted(idx); });

  connect(GradientWidget, &ezQtColorGradientWidget::addIntensityCp, this,
          [this](double x, float intensity) { Q_EMIT IntensityCpAdded(x, intensity); });
  connect(GradientWidget, &ezQtColorGradientWidget::moveIntensityCpToPos, this,
          [this](ezInt32 idx, double x) { Q_EMIT IntensityCpMoved(idx, x); });
  connect(GradientWidget, &ezQtColorGradientWidget::deleteIntensityCp, this, [this](ezInt32 idx) { Q_EMIT IntensityCpDeleted(idx); });

  connect(GradientWidget, &ezQtColorGradientWidget::beginOperation, this, [this]() { Q_EMIT BeginOperation(); });
  connect(GradientWidget, &ezQtColorGradientWidget::endOperation, this, [this](bool commit) { Q_EMIT EndOperation(commit); });

  connect(GradientWidget, &ezQtColorGradientWidget::triggerPickColor, this, [this]() { on_ButtonColor_clicked(); });
}


ezQtColorGradientEditorWidget::~ezQtColorGradientEditorWidget() {}


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

  // const bool wasEmpty = m_Gradient.IsEmpty();

  m_Gradient = gradient;

  {
    ezQtScopedUpdatesDisabled ud(this);

    // if (wasEmpty)
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
    Q_EMIT ColorCpMoved(m_iSelectedColorCP, value);
  }
  else if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpMoved(m_iSelectedAlphaCP, value);
  }
  else if (m_iSelectedIntensityCP != -1)
  {
    Q_EMIT IntensityCpMoved(m_iSelectedIntensityCP, value);
  }
}


void ezQtColorGradientEditorWidget::on_SpinAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}

void ezQtColorGradientEditorWidget::on_SliderAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}


void ezQtColorGradientEditorWidget::on_SliderAlpha_sliderPressed()
{
  Q_EMIT BeginOperation();
}


void ezQtColorGradientEditorWidget::on_SliderAlpha_sliderReleased()
{
  Q_EMIT EndOperation(true);
}

void ezQtColorGradientEditorWidget::on_SpinIntensity_valueChanged(double value)
{
  if (m_iSelectedIntensityCP != -1)
  {
    Q_EMIT IntensityCpChanged(m_iSelectedIntensityCP, value);
  }
}


void ezQtColorGradientEditorWidget::on_ButtonColor_clicked()
{
  if (m_iSelectedColorCP != -1)
  {
    const auto& cp = m_Gradient.GetColorControlPoint(m_iSelectedColorCP);

    m_PickColorStart = ezColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
    m_PickColorCurrent = m_PickColorStart;

    Q_EMIT BeginOperation();

    ezQtUiServices::GetSingleton()->ShowColorDialog(m_PickColorStart, false, false, this, SLOT(onCurrentColorChanged(const ezColor&)),
                                                    SLOT(onColorAccepted()), SLOT(onColorReset()));
  }
}

void ezQtColorGradientEditorWidget::onCurrentColorChanged(const ezColor& col)
{
  if (m_iSelectedColorCP != -1)
  {
    m_PickColorCurrent = col;

    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);
  }
}

void ezQtColorGradientEditorWidget::onColorAccepted()
{
  if (m_iSelectedColorCP != -1)
  {
    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);

    Q_EMIT EndOperation(true);
  }
}

void ezQtColorGradientEditorWidget::onColorReset()
{
  if (m_iSelectedColorCP != -1)
  {
    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorStart);

    Q_EMIT EndOperation(false);
  }
}


void ezQtColorGradientEditorWidget::on_ButtonNormalize_clicked()
{
  Q_EMIT NormalizeRange();
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
