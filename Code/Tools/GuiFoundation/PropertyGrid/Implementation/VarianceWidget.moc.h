#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/Types/VarianceTypes.h>

class QSlider;

class ezQtVarianceTypeWidget : public ezQtEmbeddedClassPropertyWidget
{
  Q_OBJECT

public:
  ezQtVarianceTypeWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

private slots:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();
  void SlotVarianceChanged();

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  virtual void OnPropertyChanged(const ezString& sProperty) override;

  QHBoxLayout* m_pLayout = nullptr;
  ezQtDoubleSpinBox* m_pValueWidget = nullptr;
  QSlider* m_pVarianceWidget = nullptr;
  const ezRTTI* m_pValueType = nullptr;
};
