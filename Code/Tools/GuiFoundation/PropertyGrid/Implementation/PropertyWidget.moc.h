#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

#include <QFrame>

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
class ezDocumentObjectBase;
class QDoubleSpinBoxLessAnnoying;

/// *** CHECKBOX ***

class EZ_GUIFOUNDATION_DLL ezPropertyEditorCheckboxWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorCheckboxWidget();

  virtual void mousePressEvent(QMouseEvent * ev) override;

private slots:
  void on_StateChanged_triggered(int state);

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const ezVariant& value) override;

  QHBoxLayout* m_pLayout;
  QCheckBox* m_pWidget;
  
};



/// *** DOUBLE SPINBOX ***

class EZ_GUIFOUNDATION_DLL ezPropertyEditorDoubleSpinboxWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorDoubleSpinboxWidget(ezInt8 iNumComponents);

private slots:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand;
  ezInt8 m_iNumComponents;
  QHBoxLayout* m_pLayout;
  QDoubleSpinBoxLessAnnoying*  m_pWidget[4];
};

/// *** INT SPINBOX ***

class EZ_GUIFOUNDATION_DLL ezPropertyEditorIntSpinboxWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorIntSpinboxWidget(ezInt32 iMinValue, ezInt32 iMaxValue);

private slots:
  void SlotValueChanged();
  void on_EditingFinished_triggered();

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  QSpinBox* m_pWidget;
};

/// *** DOUBLE SPINBOX ***

class EZ_GUIFOUNDATION_DLL ezPropertyEditorQuaternionWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorQuaternionWidget();

private slots:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  QDoubleSpinBox*  m_pWidget[3];
};


/// *** LINEEDIT ***

class EZ_GUIFOUNDATION_DLL ezPropertyEditorLineEditWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorLineEditWidget();

protected slots:
  void on_TextChanged_triggered(const QString& value);
  void on_TextFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QLineEdit* m_pWidget;
};


/// *** File Browser ***

class EZ_GUIFOUNDATION_DLL ezPropertyEditorFileBrowserWidget : public ezPropertyEditorLineEditWidget
{
  Q_OBJECT

public:
  ezPropertyEditorFileBrowserWidget();

private slots:
  void on_BrowseFile_clicked();

protected:
  virtual void OnInit() override;

protected:
  QToolButton* m_pButton;
};


/// *** COLOR ***

class EZ_GUIFOUNDATION_DLL ezColorButton : public QFrame
{
  Q_OBJECT

public:
  explicit ezColorButton(QWidget* parent);
  void SetColor(const ezVariant& color);

signals:
  void clicked();

protected:
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

};

class EZ_GUIFOUNDATION_DLL ezPropertyEditorColorWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorColorWidget();

private slots:
  void on_Button_triggered();
  void on_CurrentColor_changed(const QColor& color);
  void on_Color_reset();
  void on_Color_accepted();

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  ezColorButton* m_pWidget;
  ezVariant m_OriginalValue;
};


/// *** ENUM COMBOBOX ***

class EZ_GUIFOUNDATION_DLL ezPropertyEditorEnumWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorEnumWidget();

private slots:
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

class EZ_GUIFOUNDATION_DLL ezPropertyEditorBitflagsWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorBitflagsWidget();
  virtual ~ezPropertyEditorBitflagsWidget();

private slots:
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