#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/VarianceTypes.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>

class QSlider;

class ezQtVarianceTypeWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtVarianceTypeWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

private Q_SLOTS:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();
  void SlotVarianceChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  ezQtDoubleSpinBox* m_pValueWidget = nullptr;
  QSlider* m_pVarianceWidget = nullptr;
  const ezAbstractMemberProperty* m_pValueProp = nullptr;
  const ezAbstractMemberProperty* m_pVarianceProp = nullptr;
};
