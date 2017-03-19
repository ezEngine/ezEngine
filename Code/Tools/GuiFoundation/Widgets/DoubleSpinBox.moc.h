#pragma once

#include <GuiFoundation/Basics.h>
#include <QDoubleSpinBox>
class ezVariant;

class EZ_GUIFOUNDATION_DLL ezQtDoubleSpinBox : public QDoubleSpinBox
{
  Q_OBJECT
public:
  explicit ezQtDoubleSpinBox(QWidget* pParent, bool bIntMode = false);

  void SetIntMode(bool enable);

  void setDisplaySuffix(const char* szSuffix);
  void setDefaultValue(double value);
  void setDefaultValue(const ezVariant& val);
  using QDoubleSpinBox::setMinimum;
  using QDoubleSpinBox::setMaximum;
  void setMinimum(const ezVariant& val);
  void setMaximum(const ezVariant& val);

  virtual QString textFromValue(double val) const override;
  virtual double valueFromText(const QString &text) const override;

  void setValueInvalid();
  void setValue(double val);
  void setValue(const ezVariant& val);
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
  bool m_bModified;
  bool m_bIntMode;
  bool m_bDragging;
  double m_fStartDragValue;
  QPoint m_LastDragPos;
  ezInt32 m_iDragDelta;
};
