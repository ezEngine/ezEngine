#pragma once

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ColorGradientEditorWidget.h>

#include <QWidget>

class QMouseEvent;

class EZ_GUIFOUNDATION_DLL ezQtColorGradientEditorWidget : public QWidget, public Ui_ColorGradientEditorWidget
{
  Q_OBJECT

public:
  explicit ezQtColorGradientEditorWidget(QWidget* pParent);
  ~ezQtColorGradientEditorWidget();

  void SetColorGradient(const ezColorGradient& gradient);
  const ezColorGradient& GetColorGradient() const { return m_Gradient; }

  void ShowColorPicker() { on_ButtonColor_clicked(); }
  void SetScrubberPosition(ezUInt64 uiTick);

  void FrameGradient();

Q_SIGNALS:
  void ColorCpAdded(double fPosX, const ezColorGammaUB& color);
  void ColorCpMoved(ezInt32 iIndex, float fNewPosX);
  void ColorCpDeleted(ezInt32 iIndex);
  void ColorCpChanged(ezInt32 iIndex, const ezColorGammaUB& color);

  void AlphaCpAdded(double fPosX, ezUInt8 uiAlpha);
  void AlphaCpMoved(ezInt32 iIndex, double fNewPosX);
  void AlphaCpDeleted(ezInt32 iIndex);
  void AlphaCpChanged(ezInt32 iIndex, ezUInt8 uiAlpha);

  void IntensityCpAdded(double fPosX, float fIntensity);
  void IntensityCpMoved(ezInt32 iIndex, double fNewPosX);
  void IntensityCpDeleted(ezInt32 iIndex);
  void IntensityCpChanged(ezInt32 iIndex, float fIntensity);

  void NormalizeRange();

  void BeginOperation();
  void EndOperation(bool bCommit);

private Q_SLOTS:
  void on_ButtonFrame_clicked();
  void on_GradientWidget_selectionChanged(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP);
  void on_SpinPosition_valueChanged(double value);
  void on_SpinAlpha_valueChanged(int value);
  void on_SliderAlpha_valueChanged(int value);
  void on_SliderAlpha_sliderPressed();
  void on_SliderAlpha_sliderReleased();
  void on_SpinIntensity_valueChanged(double value);
  void on_ButtonColor_clicked();
  void onCurrentColorChanged(const ezColor& col);
  void onColorAccepted();
  void onColorReset();
  void on_ButtonNormalize_clicked();

protected:
  virtual void showEvent(QShowEvent* event) override;

private:
  void UpdateCpUi();

  QPalette m_Pal;
  ezInt32 m_iSelectedColorCP;
  ezInt32 m_iSelectedAlphaCP;
  ezInt32 m_iSelectedIntensityCP;
  ezColorGradient m_Gradient;

  ezColorGammaUB m_PickColorStart;
  ezColorGammaUB m_PickColorCurrent;
};
