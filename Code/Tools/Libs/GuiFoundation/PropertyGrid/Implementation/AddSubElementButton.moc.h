#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;

/// \brief Used by container widgets to add new elements to the container.
class EZ_GUIFOUNDATION_DLL ezQtAddSubElementButton : public ezQtPropertyWidget
{
  Q_OBJECT

public:
  /// Constructor
  /// \param containerCategory The type of container. Only Map, Set and Array are supported.
  ezQtAddSubElementButton(ezEnum<ezPropertyCategory> containerCategory, ezStringView sButtonText);

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void onMenuAboutToShow();
  void on_Button_clicked();
  void OnTypeSelected(QString sTypeName);

private:
  virtual void OnInit() override;
  void OnAction(const ezRTTI* pRtti);

  QHBoxLayout* m_pLayout;
  QPushButton* m_pButton;

  ezQtTypeMenu m_TypeMenu;

  ezEnum<ezPropertyCategory> m_ContainerCategory;
  bool m_bNoMoreElementsAllowed = false;
  QMenu* m_pMenu = nullptr;
  ezUInt32 m_uiMaxElements = 0; // 0 means unlimited
  bool m_bPreventDuplicates = false;
};
