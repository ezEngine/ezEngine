#include <GuiFoundation/Basics.h>
#include <Foundation/Math/Color.h>
#include <Code/Tools/GuiFoundation/ui_ColorDialog.h>
#include <GuiFoundation/UIServices/ColorDlgWidgets.moc.h>
#include <QDialog>

class QLineEdit;
class ezQtDoubleSpinBox;
class QPushButton;
class QSlider;


class EZ_GUIFOUNDATION_DLL ezQtColorDialog : public QDialog, Ui_ColorDialog
{
  Q_OBJECT
public:

  ezQtColorDialog(const ezColor& initial, QWidget* parent);

  void ShowAlpha(bool enable);
  void ShowHDR(bool enable);


signals:
  void CurrentColorChanged(const ezColor& color);
  void ColorSelected(const ezColor& color);

private slots:
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

  ezUInt16 m_uiHue;
  ezUInt8 m_uiSaturation;
  ezUInt8 m_uiValue;

  ezUInt8 m_GammaRed;
  ezUInt8 m_GammaGreen;
  ezUInt8 m_GammaBlue;

  ezUInt8 m_Alpha;
  ezUInt32 m_uiExposureValue;

  ezColor m_CurrentColor;

private:
  void ApplyColor();

  void RecomputeHDR();

  void ExtractColorRGB();
  void ExtractColorHSV();

  void ComputeRgbAndHsv(const ezColor& color);
  void RecomputeRGB();
  void RecomputeHSV();
};


