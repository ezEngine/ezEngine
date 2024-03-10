#pragma once

#include <Foundation/Math/Color.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/ColorDlgWidgets.moc.h>
#include <GuiFoundation/ui_ColorDialog.h>
#include <QDialog>

class QLineEdit;
class ezQtDoubleSpinBox;
class QPushButton;
class QSlider;


class EZ_GUIFOUNDATION_DLL ezQtColorDialog : public QDialog, Ui_ColorDialog
{
  Q_OBJECT
public:
  ezQtColorDialog(const ezColor& initial, QWidget* pParent);
  ~ezQtColorDialog();

  void ShowAlpha(bool bEnable);
  void ShowHDR(bool bEnable);

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

Q_SIGNALS:
  void CurrentColorChanged(const ezColor& color);
  void ColorSelected(const ezColor& color);

private Q_SLOTS:
  void ChangedRGB();
  void ChangedAlpha();
  void ChangedExposure();
  void ChangedHSV();
  void ChangedArea(double x, double y);
  void ChangedRange(double x);
  void ChangedHEX();

private:
  bool m_bAlpha;
  bool m_bHDR;

  float m_fHue;
  float m_fSaturation;
  float m_fValue;

  ezUInt16 m_uiHue;
  ezUInt8 m_uiSaturation;

  ezUInt8 m_uiGammaRed;
  ezUInt8 m_uiGammaGreen;
  ezUInt8 m_uiGammaBlue;

  ezUInt8 m_uiAlpha;
  float m_fExposureValue;

  ezColor m_CurrentColor;

  static QByteArray s_LastDialogGeometry;

private:
  void ApplyColor();

  void RecomputeHDR();

  void ExtractColorRGB();
  void ExtractColorHSV();

  void ComputeRgbAndHsv(const ezColor& color);
  void RecomputeRGB();
  void RecomputeHSV();
};
