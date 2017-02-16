#include <PCH.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/UIServices/ColorDialog.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <QColor>
#include <QColorDialog>

QPoint ezQtColorDialog::s_LastDialogPosition;

void ezQtUiServices::ShowColorDialog(const ezColor& color, bool bAlpha, bool bHDR, QWidget* pParent, const char* slotCurColChanged, const char* slotAccept, const char* slotReject)
{
  m_pColorDlg = new ezQtColorDialog(color, pParent);
  m_pColorDlg->move(m_ColorDlgPos);
  m_pColorDlg->ShowAlpha(bAlpha);
  m_pColorDlg->ShowHDR(bHDR);

  EZ_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(CurrentColorChanged(const ezColor&)), pParent, slotCurColChanged) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(accepted()), pParent, slotAccept) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(rejected()), pParent, slotReject) != nullptr, "signal/slot connection failed");

  m_pColorDlg->exec();
  delete m_pColorDlg;
  m_pColorDlg = nullptr;

  m_ColorDlgPos = ezQtColorDialog::GetLastDialogPosition();
}

ezQtColorDialog::ezQtColorDialog(const ezColor& initial, QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  m_CurrentColor = initial;
  m_bAlpha = false;
  m_bHDR = false;

  ComputeRgbAndHsv(initial);

  {
    SpinRed->SetIntMode(true);
    SpinGreen->SetIntMode(true);
    SpinBlue->SetIntMode(true);
    SpinAlpha->SetIntMode(true);

    SpinHue->SetIntMode(true);
    SpinSaturation->SetIntMode(true);
    SpinValue->SetIntMode(true);

    ButtonOk->setAutoDefault(false);
    ButtonCancel->setAutoDefault(false);

    ButtonOk->setDefault(false);
    ButtonCancel->setDefault(false);
  }

  {
    connect(SpinHue, SIGNAL(valueChanged(double)), this, SLOT(ChangedHSV()));
    connect(SpinSaturation, SIGNAL(valueChanged(double)), this, SLOT(ChangedHSV()));
    connect(SpinValue, SIGNAL(valueChanged(double)), this, SLOT(ChangedHSV()));
    connect(SpinRed, SIGNAL(valueChanged(double)), this, SLOT(ChangedRGB()));
    connect(SpinGreen, SIGNAL(valueChanged(double)), this, SLOT(ChangedRGB()));
    connect(SpinBlue, SIGNAL(valueChanged(double)), this, SLOT(ChangedRGB()));
    connect(SpinAlpha, SIGNAL(valueChanged(double)), this, SLOT(ChangedAlpha()));
    connect(SliderExposure, SIGNAL(valueChanged(int)), this, SLOT(ChangedExposure()));
    connect(ColorArea, SIGNAL(valueChanged(double, double)), this, SLOT(ChangedArea(double, double)));
    connect(ColorRange, SIGNAL(valueChanged(double)), this, SLOT(ChangedRange(double)));
    connect(LineHEX, SIGNAL(editingFinished()), this, SLOT(ChangedHEX()));
    connect(ButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
  }

  ColorCompare->SetInitialColor(initial);

  ShowAlpha(m_bAlpha);
  ShowHDR(m_bHDR);
  ApplyColor();
}

ezQtColorDialog::~ezQtColorDialog()
{
  s_LastDialogPosition = pos();
}

void ezQtColorDialog::ShowAlpha(bool enable)
{
  m_bAlpha = enable;
  SpinAlpha->setVisible(enable);
  LabelAlpha->setVisible(enable);

  ApplyColor();
}

void ezQtColorDialog::ShowHDR(bool enable)
{
  m_bHDR = enable;
  LineRed32->setVisible(enable);
  LineGreen32->setVisible(enable);
  LineBlue32->setVisible(enable);
  LabelExposure->setVisible(enable);
  LineExposure->setVisible(enable);
  SliderExposure->setVisible(enable);
  LabelR32->setVisible(enable);
  LabelG32->setVisible(enable);
  LabelB32->setVisible(enable);

  ApplyColor();
}

void ezQtColorDialog::ApplyColor()
{
  ezQtScopedBlockSignals _0(SpinAlpha, SliderExposure, LineExposure);
  ezQtScopedBlockSignals _1(SpinRed, SpinGreen, SpinBlue);
  ezQtScopedBlockSignals _2(SpinHue, SpinSaturation, SpinValue);
  ezQtScopedBlockSignals _4(LineRed32, LineGreen32, LineBlue32);
  ezQtScopedBlockSignals _3(ColorRange, ColorArea);

  SpinAlpha->setValue(m_Alpha);
  SliderExposure->setValue(m_uiExposureValue);
  LineExposure->setText(QString("+%1").arg(m_uiExposureValue));

  SpinRed->setValue(m_GammaRed);
  SpinGreen->setValue(m_GammaGreen);
  SpinBlue->setValue(m_GammaBlue);

  SpinHue->setValue(m_uiHue);
  SpinSaturation->setValue(m_uiSaturation);
  SpinValue->setValue(m_uiValue);

  LineRed32->setText(QString("%1").arg(m_CurrentColor.r, 0, 'f', 2));
  LineGreen32->setText(QString("%1").arg(m_CurrentColor.g, 0, 'f', 2));
  LineBlue32->setText(QString("%1").arg(m_CurrentColor.b, 0, 'f', 2));

  ColorRange->SetHue(m_uiHue);
  ColorArea->SetHue(m_uiHue);
  ColorArea->SetSaturation(m_uiSaturation / 100.0f);
  ColorArea->SetValue(m_uiValue / 100.0f);
  ColorCompare->SetNewColor(m_CurrentColor);

  ezStringBuilder s;

  if (m_bAlpha)
  {
    s.Format("{0}{1}{2}{3}", ezArgU(m_GammaRed, 2, true, 16, true), ezArgU(m_GammaGreen, 2, true, 16, true), ezArgU(m_GammaBlue, 2, true, 16, true), ezArgU(m_Alpha, 2, true, 16, true));
  }
  else
  {
    s.Format("{0}{1}{2}", ezArgU(m_GammaRed, 2, true, 16, true), ezArgU(m_GammaGreen, 2, true, 16, true), ezArgU(m_GammaBlue, 2, true, 16, true));
  }

  LineHEX->setText(s.GetData());
}

void ezQtColorDialog::ChangedRGB()
{
  ExtractColorRGB();
  ApplyColor();
  emit CurrentColorChanged(m_CurrentColor);
}

void ezQtColorDialog::ChangedHSV()
{
  ExtractColorHSV();
  ApplyColor();
  emit CurrentColorChanged(m_CurrentColor);
}

void ezQtColorDialog::ChangedAlpha()
{
  m_Alpha = (ezUInt8)SpinAlpha->value();
  m_CurrentColor.a = ezMath::ColorByteToFloat(m_Alpha);

  ApplyColor();
  emit CurrentColorChanged(m_CurrentColor);
}

void ezQtColorDialog::ChangedExposure()
{
  m_uiExposureValue = SliderExposure->value();

  RecomputeHDR();
  ApplyColor();
  emit CurrentColorChanged(m_CurrentColor);
}

void ezQtColorDialog::ChangedArea(double x, double y)
{
  m_uiSaturation = ezMath::Min<ezUInt8>(ezMath::Round(x * 100.0), 100);
  m_uiValue = ezMath::Min<ezUInt8>(ezMath::Round(y * 100.0), 100);

  RecomputeRGB();
  RecomputeHDR();

  ApplyColor();
  emit CurrentColorChanged(m_CurrentColor);
}

void ezQtColorDialog::ChangedRange(double x)
{
  m_uiHue = ezMath::Clamp<ezUInt16>(ezMath::Round(x * 359), 0, 359);

  RecomputeRGB();
  RecomputeHDR();

  ApplyColor();
  emit CurrentColorChanged(m_CurrentColor);
}

void ezQtColorDialog::ChangedHEX()
{
  ezStringBuilder text = LineHEX->text().toUtf8().data();

  if (!text.StartsWith("#"))
  {
    text.Prepend("#");
  }

  bool valid = false;
  ezColor col = ezConversionUtils::GetColorByName(text, &valid);

  if (valid)
  {
    ComputeRgbAndHsv(col);
    RecomputeHDR();
  }

  ApplyColor();

  if (valid)
  {
    emit CurrentColorChanged(m_CurrentColor);
  }
}

void ezQtColorDialog::ExtractColorRGB()
{
  m_GammaRed = (ezUInt8)SpinRed->value();
  m_GammaGreen = (ezUInt8)SpinGreen->value();
  m_GammaBlue = (ezUInt8)SpinBlue->value();

  RecomputeHSV();
  RecomputeHDR();
}

void ezQtColorDialog::ExtractColorHSV()
{
  m_uiHue = ezMath::Clamp<float>(SpinHue->value(), 0, 359);
  m_uiSaturation = ezMath::Min<ezUInt8>(SpinSaturation->value(), 100);
  m_uiValue = ezMath::Min<ezUInt8>(SpinValue->value(), 100);

  RecomputeRGB();
  RecomputeHDR();
}

void ezQtColorDialog::ComputeRgbAndHsv(const ezColor& color)
{
  m_uiExposureValue = color.ComputeHdrExposureValue();

  ezColorGammaUB gamma = color;
  m_GammaRed = gamma.r;
  m_GammaGreen = gamma.g;
  m_GammaBlue = gamma.b;
  m_Alpha = gamma.a;

  RecomputeHSV();
}

void ezQtColorDialog::RecomputeRGB()
{
  ezColor col;
  col.SetHSV(m_uiHue, m_uiSaturation / 100.0, m_uiValue / 100.0);
  ezColorGammaUB colGamma = col;

  m_GammaRed = colGamma.r;
  m_GammaGreen = colGamma.g;
  m_GammaBlue = colGamma.b;
}

void ezQtColorDialog::RecomputeHSV()
{
  ezColorGammaUB colGamma(m_GammaRed, m_GammaGreen, m_GammaBlue, 255);
  ezColor color = colGamma;

  float hue, sat, val;
  color.GetHSV(hue, sat, val);

  m_uiHue = (ezUInt16)ezMath::Round(hue);
  m_uiSaturation = ezMath::Min<ezUInt8>(ezMath::Round(sat * 100.0), 100);
  m_uiValue = ezMath::Min<ezUInt8>(ezMath::Round(val * 100.0), 100);
}

void ezQtColorDialog::RecomputeHDR()
{
  //ezColorGammaUB gamma(m_GammaRed, m_GammaGreen, m_GammaBlue, m_Alpha);
  //m_CurrentColor = gamma;
  //m_CurrentColor.ApplyHdrExposureValue(m_uiExposureValue);

  // not sure whether this is better than just scaling RGB, no test data yet
  m_CurrentColor.SetHSV(m_uiHue, m_uiSaturation / 100.0, (m_uiValue / 100.0) * (1U << m_uiExposureValue));
}
