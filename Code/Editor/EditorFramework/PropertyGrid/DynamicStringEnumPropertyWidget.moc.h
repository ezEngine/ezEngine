#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class ezDynamicStringEnum;
class ezQtSearchableMenu;

class EZ_EDITORFRAMEWORK_DLL ezQtDynamicStringEnumPropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtDynamicStringEnumPropertyWidget();

protected slots:
  void onMenuAboutToShow();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  void SetNewValue(ezStringView sNewValue);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  ezDynamicStringEnum* m_pEnum = nullptr;
  QPushButton* m_pButton = nullptr;
  QMenu* m_pMenu = nullptr;
  ezQtSearchableMenu* m_pSearchableMenu = nullptr;
  ezString m_sEnumAttribute;
  static ezMap<ezString, QString> s_LastSearch;
};
