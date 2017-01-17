#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <ParticlePlugin/Util/ParticleUtils.h>

class ezQtVarianceTypeWidget : public ezQtPropertyWidget
{
  Q_OBJECT

public:
  ezQtVarianceTypeWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;

private slots:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override {}

  virtual void DoPrepareToDie() override {}

  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  ezQtDoubleSpinBox* m_pValueWidget;
  ezQtDoubleSpinBox* m_pVarianceWidget;
};
