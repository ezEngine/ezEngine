#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

class QSlider;

class ezQtExposedBoneWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtExposedBoneWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

private Q_SLOTS:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  ezQtDoubleSpinBox* m_pRotWidget[3];
};
