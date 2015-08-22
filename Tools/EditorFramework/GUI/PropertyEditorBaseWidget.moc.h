#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <QWidget>

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
/// *** BASE ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorBaseWidget : public QWidget
{
public:
  ezPropertyEditorBaseWidget(const char* szName, QWidget* pParent);
  virtual ~ezPropertyEditorBaseWidget();

  struct Selection
  {
    const ezDocumentObjectBase* m_pObject;
    ezVariant m_Index;
  };

  void Init(const ezHybridArray<Selection, 8>& items, const ezPropertyPath& path);


public:
  virtual void InternalSetValue(const ezVariant& value) = 0;

  struct Event
  {
    enum class Type
    {
      ValueChanged,
      BeginTemporary,
      EndTemporary,
      CancelTemporary,
    };

    Type m_Type;
    const ezPropertyPath* m_pPropertyPath;
    const ezHybridArray<Selection, 8>* m_pItems;
    ezVariant m_Value;
  };

  ezEvent<const Event&> m_Events;

protected:
  void Broadcast(Event::Type type);
  void BroadcastValueChanged(const ezVariant& NewValue);

  virtual void OnInit() {}

  const char* m_szDisplayName;
  ezPropertyPath m_PropertyPath;
  ezHybridArray<Selection, 8> m_Items;

private:
  ezVariant m_OldValue;
};



/// *** CHECKBOX ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorCheckboxWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorCheckboxWidget(const char* szName, QWidget* pParent);

  virtual void mousePressEvent(QMouseEvent * ev) override;

private slots:
  void on_StateChanged_triggered(int state);

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QCheckBox* m_pWidget;
  
};



/// *** DOUBLE SPINBOX ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorDoubleSpinboxWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorDoubleSpinboxWidget(const char* szName, QWidget* pParent, ezInt8 iNumComponents);

private slots:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand;
  ezInt8 m_iNumComponents;
  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QDoubleSpinBox*  m_pWidget[4];
};

/// *** INT SPINBOX ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorIntSpinboxWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorIntSpinboxWidget(const char* szName, QWidget* pParent, ezInt32 iMinValue, ezInt32 iMaxValue);

private slots:
  void SlotValueChanged();
  void on_EditingFinished_triggered();

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QSpinBox* m_pWidget;
};

/// *** DOUBLE SPINBOX ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorQuaternionWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorQuaternionWidget(const char* szName, QWidget* pParent);

private slots:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QDoubleSpinBox*  m_pWidget[3];
};


/// *** LINEEDIT ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorLineEditWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorLineEditWidget(const char* szName, QWidget* pParent);

private slots:
  void on_TextChanged_triggered(const QString& value);
  void on_TextFinished_triggered();
  void on_BrowseFile_clicked();

private:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;
  virtual void focusOutEvent(QFocusEvent* event) override;

  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QLineEdit* m_pWidget;
  QToolButton* m_pButton;

  const ezFileBrowserAttribute* m_pFileAttribute;
  const ezAssetBrowserAttribute* m_pAssetAttribute;
};


/// *** COLOR ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorColorWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorColorWidget(const char* szName, QWidget* pParent);

private slots:
  void on_Button_triggered();
  void on_CurrentColor_changed(const QColor& color);
  void on_Color_reset();
  void on_Color_accepted();

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QPushButton* m_pWidget;
  ezColor m_CurrentColor;
};


/// *** ENUM COMBOBOX ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorEnumWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorEnumWidget(const char* szName, QWidget* pParent, const ezRTTI* enumType);

private slots:
  void on_CurrentEnum_changed(int iEnum);

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QComboBox* m_pWidget;
  ezInt64 m_iCurrentEnum;
};


/// *** BITFLAGS COMBOBOX ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorBitflagsWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorBitflagsWidget(const char* szName, QWidget* pParent, const ezRTTI* enumType);
  virtual ~ezPropertyEditorBitflagsWidget();

private slots:
  void on_Menu_aboutToShow();
  void on_Menu_aboutToHide();

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  
  ezMap<ezInt64, QCheckBox*> m_Constants;
  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QPushButton* m_pWidget;
  QMenu* m_pMenu;
  ezInt64 m_iCurrentBitflags;
};