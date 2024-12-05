#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

class QHBoxLayout;

class EZ_EDITORFRAMEWORK_DLL ezQtRttiTypeStringPropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtRttiTypeStringPropertyWidget();

protected Q_SLOTS:
  void onMenuAboutToShow();
  void OnTypeSelected(QString sTypeName);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  QPushButton* m_pButton = nullptr;
  QHBoxLayout* m_pLayout = nullptr;
  QMenu* m_pMenu = nullptr;

  ezQtTypeMenu m_TypeMenu;
};
