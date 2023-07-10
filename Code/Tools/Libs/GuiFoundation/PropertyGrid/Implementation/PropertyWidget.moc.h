#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

#include <QFrame>
#include <QLabel>

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QComboBox;
class QStandardItemModel;
class QStandardItem;
class QToolButton;
class QMenu;
class ezDocumentObject;
class ezQtDoubleSpinBox;
class QSlider;

/// *** CHECKBOX ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorCheckboxWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorCheckboxWidget();

  virtual void mousePressEvent(QMouseEvent* pEv) override;

private Q_SLOTS:
  void on_StateChanged_triggered(int state);

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const ezVariant& value) override;

  QHBoxLayout* m_pLayout;
  QCheckBox* m_pWidget;
};



/// *** DOUBLE SPINBOX ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorDoubleSpinboxWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorDoubleSpinboxWidget(ezInt8 iNumComponents);

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bUseTemporaryTransaction = false;
  bool m_bTemporaryCommand = false;
  ezInt8 m_iNumComponents = 0;
  ezEnum<ezVariantType> m_OriginalType;
  QHBoxLayout* m_pLayout = nullptr;
  ezQtDoubleSpinBox* m_pWidget[4] = {};
};

/// *** TIME SPINBOX ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorTimeWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorTimeWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  ezQtDoubleSpinBox* m_pWidget;
};

/// *** ANGLE SPINBOX ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorAngleWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorAngleWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  ezQtDoubleSpinBox* m_pWidget;
};

/// *** INT SPINBOX ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorIntSpinboxWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorIntSpinboxWidget(ezInt8 iNumComponents, ezInt32 iMinValue, ezInt32 iMaxValue);
  ~ezQtPropertyEditorIntSpinboxWidget();

private Q_SLOTS:
  void SlotValueChanged();
  void SlotSliderValueChanged(int value);
  void on_EditingFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bUseTemporaryTransaction = false;
  bool m_bTemporaryCommand = false;
  ezInt8 m_iNumComponents = 0;
  ezEnum<ezVariantType> m_OriginalType;
  QHBoxLayout* m_pLayout = nullptr;
  ezQtDoubleSpinBox* m_pWidget[4] = {};
  QSlider* m_pSlider = nullptr;
};

/// *** QUATERNION ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorQuaternionWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorQuaternionWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  ezQtDoubleSpinBox* m_pWidget[3];
};


/// *** LINEEDIT ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorLineEditWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorLineEditWidget();

protected Q_SLOTS:
  void on_TextChanged_triggered(const QString& value);
  void on_TextFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QLineEdit* m_pWidget;
  ezEnum<ezVariantType> m_OriginalType;
};


/// *** COLOR ***

class EZ_GUIFOUNDATION_DLL ezQtColorButtonWidget : public QFrame
{
  Q_OBJECT

public:
  explicit ezQtColorButtonWidget(QWidget* pParent);
  void SetColor(const ezVariant& color);

Q_SIGNALS:
  void clicked();

protected:
  virtual void showEvent(QShowEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private:
  QPalette m_Pal;
};

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorColorWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorColorWidget();

private Q_SLOTS:
  void on_Button_triggered();
  void on_CurrentColor_changed(const ezColor& color);
  void on_Color_reset();
  void on_Color_accepted();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  bool m_bExposeAlpha;
  QHBoxLayout* m_pLayout;
  ezQtColorButtonWidget* m_pWidget;
  ezVariant m_OriginalValue;
};


/// *** ENUM COMBOBOX ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorEnumWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorEnumWidget();

private Q_SLOTS:
  void on_CurrentEnum_changed(int iEnum);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QComboBox* m_pWidget;
  ezInt64 m_iCurrentEnum;
};


/// *** BITFLAGS COMBOBOX ***

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorBitflagsWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorBitflagsWidget();
  virtual ~ezQtPropertyEditorBitflagsWidget();

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void on_Menu_aboutToHide();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  ezMap<ezInt64, QCheckBox*> m_Constants;
  QHBoxLayout* m_pLayout;
  QPushButton* m_pWidget;
  QMenu* m_pMenu;
  ezInt64 m_iCurrentBitflags;
};


/// *** CURVE1D ***

class EZ_GUIFOUNDATION_DLL ezQtCurve1DButtonWidget : public QLabel
{
  Q_OBJECT

public:
  explicit ezQtCurve1DButtonWidget(QWidget* pParent);

  void UpdatePreview(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange);

Q_SIGNALS:
  void clicked();

protected:
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
};

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorCurve1DWidget : public ezQtPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorCurve1DWidget();

private Q_SLOTS:
  void on_Button_triggered();

protected:
  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  void UpdatePreview();

protected:
  QHBoxLayout* m_pLayout = nullptr;
  ezQtCurve1DButtonWidget* m_pButton = nullptr;
};
