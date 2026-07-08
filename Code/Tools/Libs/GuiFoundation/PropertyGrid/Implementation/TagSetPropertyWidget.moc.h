#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;
class QCheckBox;

class EZ_GUIFOUNDATION_DLL ezQtPropertyEditorTagSetWidget : public ezQtPropertyWidget
{
  Q_OBJECT

public:
  ezQtPropertyEditorTagSetWidget();
  virtual ~ezQtPropertyEditorTagSetWidget();

  virtual void SetSelection(const ezArrayPtr<ezPropertySelection>& items) override;
  virtual bool HasLabel() const override { return true; }

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void onCheckBoxClicked(bool bChecked);
  void onRemoveInvalidTagsClicked();

private:
  virtual void OnInit() override;
  void InternalUpdateValue();

private:
  ezDynamicArray<QCheckBox*> m_Tags;
  ezDynamicArray<QAction*> m_InvalidTagActions; ///< Rebuilt every InternalUpdateValue(), since which tags are "invalid" depends on the current selection.
  QAction* m_pInvalidTagsAnchor = nullptr;      ///< Dynamic invalid-tag actions are inserted right before this one.
  QHBoxLayout* m_pLayout;
  QPushButton* m_pWidget;
  QMenu* m_pMenu;
  ezString m_sTagFilter;
};
