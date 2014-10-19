#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <QWidget>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QHBoxLayout;
class QLineEdit;

/// *** BASE ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorBaseWidget : public QWidget
{
public:
  ezPropertyEditorBaseWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent);

  void SetValue(const ezVariant& value);

public:
  virtual void InternalSetValue(const ezVariant& value) = 0;

  struct EventData
  {
    const ezPropertyPath* m_pPropertyPath;
    ezVariant m_Value;
  };

  ezEvent<const EventData&> m_ValueChanged;

protected:
  void BroadcastValueChanged(const ezVariant& NewValue);

  const char* m_szDisplayName;
  ezPropertyPath m_PropertyPath;

private:
  virtual void keyPressEvent(QKeyEvent* pEvent) override;

  ezVariant m_OldValue;
};



/// *** CHECKBOX ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorCheckboxWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorCheckboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent);

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
  ezPropertyEditorDoubleSpinboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent);

private slots:
  void on_ValueChanged_triggered(double value);
  void on_EditingFinished_triggered();

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QDoubleSpinBox* m_pWidget;
};


/// *** LINEEDIT ***

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorLineEditWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorLineEditWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent);

private slots:
  void on_TextChanged_triggered(const QString& value);
  void on_TextFinished_triggered();

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QLineEdit* m_pWidget;
};
