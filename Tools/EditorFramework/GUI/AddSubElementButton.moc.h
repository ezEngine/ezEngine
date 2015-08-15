#pragma once

#include <Plugin.h>
#include <QPushButton>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>

class QHBoxLayout;

class EZ_EDITORFRAMEWORK_DLL ezAddSubElementButton : public ezPropertyEditorBaseWidget
{
  Q_OBJECT

public:
  ezAddSubElementButton(const char* szName, QWidget* pParent);

private slots:
  void on_Menu_aboutToShow();
  void OnMenuAction();

private:
  virtual void InternalSetValue(const ezVariant& value) override;

  QHBoxLayout* m_pLayout;
  QPushButton* m_pButton;

  ezSet<const ezRTTI*> m_SupportedTypes;

  QMenu* m_pMenu;
};
