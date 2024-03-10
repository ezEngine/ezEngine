#pragma once

#include <Foundation/Math/Color.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class EZ_GUIFOUNDATION_DLL ezQtColorAreaWidget : public QWidget
{
  Q_OBJECT
public:
  ezQtColorAreaWidget(QWidget* pParent);

  float GetHue() const { return m_fHue; }
  void SetHue(float fHue);

  float GetSaturation() const { return m_fSaturation; }
  void SetSaturation(float fSat);

  float GetValue() const { return m_fValue; }
  void SetValue(float fVal);

Q_SIGNALS:
  void valueChanged(double x, double y);

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

  void UpdateImage();

  QImage m_Image;
  float m_fHue;
  float m_fSaturation;
  float m_fValue;
};

class EZ_GUIFOUNDATION_DLL ezQtColorRangeWidget : public QWidget
{
  Q_OBJECT
public:
  ezQtColorRangeWidget(QWidget* pParent);

  float GetHue() const { return m_fHue; }
  void SetHue(float fHue);

Q_SIGNALS:
  void valueChanged(double x);

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

  void UpdateImage();

  QImage m_Image;
  float m_fHue;
};

class EZ_GUIFOUNDATION_DLL ezQtColorCompareWidget : public QWidget
{
  Q_OBJECT
public:
  ezQtColorCompareWidget(QWidget* pParent);

  void SetNewColor(const ezColor& color);
  void SetInitialColor(const ezColor& color);

protected:
  virtual void paintEvent(QPaintEvent*) override;

  ezColor m_InitialColor;
  ezColor m_NewColor;
};
