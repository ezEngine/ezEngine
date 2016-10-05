#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;

class EZ_GUIFOUNDATION_DLL ezQtAddSubElementButton : public ezQtPropertyWidget
{
  Q_OBJECT

public:
  ezQtAddSubElementButton();

protected:
  virtual void DoPrepareToDie() override {}

private slots:
  void on_Menu_aboutToShow();
  void on_Button_clicked();
  void OnMenuAction();
 
private:
  virtual void OnInit() override;
  void OnAction(const ezRTTI* pRtti);

  QMenu* CreateCategoryMenu(const char* szCategory, ezMap<ezString, QMenu*>& existingMenus);

  QHBoxLayout* m_pLayout;
  QPushButton* m_pButton;

  ezSet<const ezRTTI*> m_SupportedTypes;

  QMenu* m_pMenu;
};
