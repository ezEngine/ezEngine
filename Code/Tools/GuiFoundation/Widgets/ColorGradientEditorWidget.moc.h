#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Math/ColorGradient.h>
#include <Code/Tools/GuiFoundation/ui_ColorGradientEditorWidget.h>

#include <QWidget>

class QMouseEvent;

class EZ_GUIFOUNDATION_DLL QColorGradientEditorWidget : public QWidget, public Ui_ColorGradientEditorWidget
{
  Q_OBJECT

public:
  explicit QColorGradientEditorWidget(QWidget* pParent);
  ~QColorGradientEditorWidget();

  void SetColorGradient(const ezColorGradient& gradient);
  const ezColorGradient& GetColorGradient() const { return m_Gradient; }

  void ShowColorPicker() { on_ButtonColor_clicked(); }

signals:
  void ColorCpAdded(float posX, const ezColorGammaUB& color);
  void ColorCpMoved(ezInt32 index, float newPosX);
  void ColorCpDeleted(ezInt32 index);
  void ColorCpChanged(ezInt32 index, const ezColorGammaUB& color);

  void AlphaCpAdded(float posX, ezUInt8 alpha);
  void AlphaCpMoved(ezInt32 index, float newPosX);
  void AlphaCpDeleted(ezInt32 index);
  void AlphaCpChanged(ezInt32 index, ezUInt8 alpha);

  void IntensityCpAdded(float posX, float intensity);
  void IntensityCpMoved(ezInt32 index, float newPosX);
  void IntensityCpDeleted(ezInt32 index);
  void IntensityCpChanged(ezInt32 index, float intensity);

  void BeginOperation();
  void EndOperation(bool commit);

private slots:
  void on_ButtonFrame_clicked();
  void on_GradientWidget_selectionChanged(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP);
  void on_SpinPosition_valueChanged(double value);
  void on_SpinAlpha_valueChanged(int value);
  void on_SliderAlpha_valueChanged(int value);
  void on_SliderAlpha_sliderPressed();
  void on_SliderAlpha_sliderReleased();
  void on_SpinIntensity_valueChanged(double value);
  void on_ButtonColor_clicked();
  void on_CurrentColor_changed(const QColor&);
  void on_Color_accepted();
  void on_Color_reset();

private:
  void UpdateCpUi();

  ezInt32 m_iSelectedColorCP;
  ezInt32 m_iSelectedAlphaCP;
  ezInt32 m_iSelectedIntensityCP;
  ezColorGradient m_Gradient;

  ezColorGammaUB m_PickColorStart;
  ezColorGammaUB m_PickColorCurrent;
};
