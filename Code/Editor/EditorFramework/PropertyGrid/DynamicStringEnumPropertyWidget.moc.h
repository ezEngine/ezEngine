#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class ezQtDynamicStringEnumMenuButton;

class EZ_EDITORFRAMEWORK_DLL ezQtDynamicStringEnumPropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtDynamicStringEnumPropertyWidget();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  void SetNewValue(ezStringView sNewValue);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  ezQtDynamicStringEnumMenuButton* m_pButton = nullptr;
};
