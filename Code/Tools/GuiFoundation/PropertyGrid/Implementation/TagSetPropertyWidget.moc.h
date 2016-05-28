#pragma once

#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;
class QCheckBox;

class EZ_GUIFOUNDATION_DLL ezPropertyEditorTagSetWidget : public ezQtPropertyWidget
{
  Q_OBJECT

public:
  ezPropertyEditorTagSetWidget();
  virtual ~ezPropertyEditorTagSetWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;
  virtual bool HasLabel() const override { return true; }

protected:
  virtual void DoPrepareToDie() override {}

private slots:
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