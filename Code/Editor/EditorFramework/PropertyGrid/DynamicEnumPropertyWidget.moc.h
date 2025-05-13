#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QComboBox;
class ezDynamicEnum;

/// *** Asset Browser ***

class EZ_EDITORFRAMEWORK_DLL ezQtDynamicEnumPropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtDynamicEnumPropertyWidget();


protected slots:
  void on_CurrentEnum_changed(int iEnum);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  QComboBox* m_pWidget;
  QHBoxLayout* m_pLayout;
  ezDynamicEnum* m_pEnum = nullptr;
  ezInt32 m_iLastIndex = -1;
};
