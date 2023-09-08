#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QComboBox;

class EZ_EDITORFRAMEWORK_DLL ezQtDynamicBitflagsPropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtDynamicBitflagsPropertyWidget();
  virtual ~ezQtDynamicBitflagsPropertyWidget();

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void on_Menu_aboutToHide();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;
  void SetAll(bool bChecked);

  void ClearMenu();
  void BuildMenu();
  void FillInCheckedBoxes();

protected:
  ezMap<ezInt64, QCheckBox*> m_Constants;
  QHBoxLayout* m_pLayout;
  QPushButton* m_pWidget;
  QPushButton* m_pAllButton;
  QPushButton* m_pClearButton;
  QHBoxLayout* m_pBottomLayout;
  QMenu* m_pMenu;
  ezInt64 m_iCurrentBitflags;
};

