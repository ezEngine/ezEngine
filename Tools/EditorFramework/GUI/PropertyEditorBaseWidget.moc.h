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

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorBaseWidget : public QWidget
{
public:
  ezPropertyEditorBaseWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent);

  virtual void SetValue(const ezVariant& value) = 0;

public:
  struct EventData
  {
    const ezPropertyPath* m_pPropertyPath;
    ezVariant m_Value;
  };

  ezEvent<const EventData&> m_ValueChanged;

protected:
  const char* m_szDisplayName;
  ezPropertyPath m_PropertyPath;
};

class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorCheckboxWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorCheckboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent);

  virtual void SetValue(const ezVariant& value) override;
  virtual void mousePressEvent(QMouseEvent * ev) override;

private slots:
  void on_StateChanged_triggered(int state);

private:
  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QCheckBox* m_pWidget;
  
};


class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorDoubleSpinboxWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorDoubleSpinboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent);

  virtual void SetValue(const ezVariant& value) override;

private slots:
  void on_ValueChanged_triggered(double value);

private:
  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QDoubleSpinBox* m_pWidget;
};


class EZ_EDITORFRAMEWORK_DLL ezPropertyEditorLineEditWidget : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezPropertyEditorLineEditWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent);

  virtual void SetValue(const ezVariant& value) override;

private slots:
  void on_TextChanged_triggered(const QString& value);

private:
  QHBoxLayout* m_pLayout;
  QLabel* m_pLabel;
  QLineEdit* m_pWidget;
};
