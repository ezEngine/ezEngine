#pragma once

#include <GuiFoundation/PCH.h>
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

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return true; }

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void onCheckBoxClicked(bool bChecked);

private:
  virtual void OnInit() override;
  void InternalUpdateValue();

private:
  ezDynamicArray<QCheckBox*> m_Tags;
  QHBoxLayout* m_pLayout;
  QPushButton* m_pWidget;
  QMenu* m_pMenu;
};