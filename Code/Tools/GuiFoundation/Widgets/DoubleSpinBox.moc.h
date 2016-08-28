#pragma once

#include <GuiFoundation/Basics.h>
#include <QDoubleSpinBox>

class EZ_GUIFOUNDATION_DLL QDoubleSpinBoxLessAnnoying : public QDoubleSpinBox
{
  Q_OBJECT
public:
  explicit QDoubleSpinBoxLessAnnoying(QWidget* pParent);

  void setDisplaySuffix(const char* szSuffix);
  void setDefaultValue(double value);

  virtual QString textFromValue(double val) const override;
  virtual double valueFromText(const QString &text) const override;

  void setValueInvalid();
  void setValue(double val);
  double value() const;

protected:
  virtual void focusInEvent(QFocusEvent* event) override;
  virtual void focusOutEvent(QFocusEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;

  private slots:
  void onCustomContextMenuRequested();

private:
  QString m_sSuffix;
  double m_fDefaultValue;
  mutable double m_fDisplayedValue;
  mutable QString m_sDisplayedText;
  mutable bool m_bInvalid;
  bool m_bDragging;
  bool m_bModified;
  QPoint m_LastDragPos;
};


class ezQIntSpinbox : public QSpinBox
{
  Q_OBJECT
public:
  explicit ezQIntSpinbox(QWidget* pParent);

  void setDefaultValue(int value) { m_iDefaultValue = value; }

private slots:
  void onCustomContextMenuRequested();

private:
  int m_iDefaultValue;
};
