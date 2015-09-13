#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;

class EZ_GUIFOUNDATION_DLL ezAddSubElementButton : public ezPropertyBaseWidget
{
  Q_OBJECT

public:
  ezAddSubElementButton();

private slots:
  void on_Menu_aboutToShow();
  void on_Button_clicked();
  void OnMenuAction();
 
private:
  virtual void OnInit() override;
  void OnAction(const ezRTTI* pRtti);

  QHBoxLayout* m_pLayout;
  QPushButton* m_pButton;

  ezSet<const ezRTTI*> m_SupportedTypes;

  QMenu* m_pMenu;
};
