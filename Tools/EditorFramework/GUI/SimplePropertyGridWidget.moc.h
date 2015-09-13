#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/HybridArray.h>
//#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <ToolsFoundation/Settings/Settings.h>
#include <Foundation/Containers/HybridArray.h>
#include <QWidget>
#include <QLayout>

class QScrollArea;

class EZ_EDITORFRAMEWORK_DLL ezSimplePropertyGridWidget : public QWidget
{
  Q_OBJECT

public:
  struct Property
  {
    ezString m_sName;
    ezVariant m_Value;
    ezVariant* m_pValue;
    QWidget* m_pWidget;
    bool m_bReadOnly;
  };

  ezSimplePropertyGridWidget(QWidget* pParent);
  ~ezSimplePropertyGridWidget();

  void BeginProperties();
  void EndProperties();
  void AddProperty(const char* szName, const ezVariant& value, ezVariant* pValue = nullptr, bool bReadOnly = false);

  void ClearProperties();

  const ezHybridArray<Property, 32>& GetAllProperties() const { return m_Properties; }

signals:
  void value_changed();

private slots:
  void SlotPropertyChanged();

private:
  void BuildUI();

  QWidget* CreateControl(Property& Prop, QWidget* pWidget);
  QWidget* CreateCheckbox(Property& Prop);
  QWidget* CreateDoubleSpinbox(Property& Prop);
  QWidget* CreateSpinbox(Property& Prop);
  QWidget* CreateLineEdit(Property& Prop);

  ezHybridArray<Property, 32> m_Properties;
  QVBoxLayout* m_pLayout;
  QScrollArea* m_pScrollArea;
  QWidget* m_pMainContent;
};


