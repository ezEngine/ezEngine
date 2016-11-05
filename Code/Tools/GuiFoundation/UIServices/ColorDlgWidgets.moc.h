#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Math/Color.h>
#include <QWidget>

class EZ_GUIFOUNDATION_DLL ezQtColorAreaWidget : public QWidget
{
  Q_OBJECT
public:
  ezQtColorAreaWidget(QWidget* parent);

  float GetHue() const { return m_fHue; }
  void SetHue(float hue);

  float GetSaturation() const { return m_fSaturation; }
  void SetSaturation(float sat);

  float GetValue() const { return m_fValue; }
  void SetValue(float val);

signals:
  void valueChanged(double x, double y);

protected:
  virtual void paintEvent(QPaintEvent *) override;
  virtual void mouseMoveEvent(QMouseEvent *) override;
  virtual void mousePressEvent(QMouseEvent *) override;

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
  ezQtColorRangeWidget(QWidget* parent);

  float GetHue() const { return m_fHue; }
  void SetHue(float hue);

signals:
  void valueChanged(double x);

protected:
  virtual void paintEvent(QPaintEvent *) override;
  virtual void mouseMoveEvent(QMouseEvent *) override;
  virtual void mousePressEvent(QMouseEvent *) override;

  void UpdateImage();

  QImage m_Image;
  float m_fHue;
};

class EZ_GUIFOUNDATION_DLL ezQtColorCompareWidget : public QWidget
{
  Q_OBJECT
public:
  ezQtColorCompareWidget(QWidget* parent);

  void SetNewColor(const ezColor& color);
  void SetInitialColor(const ezColor& color);

protected:
  virtual void paintEvent(QPaintEvent *) override;

  ezColor m_InitialColor;
  ezColor m_NewColor;
};
