#pragma once

#include <Core/Utils/Blackboard.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class QHBoxLayout;
class QComboBox;
class ezQtDoubleSpinBox;
class ezQtDynamicStringEnumMenuButton;

/// Displays all properties of ezBlackboardCondition (entry name, operator and comparison value) in a single row.
///
/// ezBlackboardCondition is a custom variant type, so the entire condition is edited as a single value. The entry
/// name reuses ezQtDynamicStringEnumMenuButton with the "BlackboardKeysEnum" dynamic string enum that is registered
/// on the reflected property.
class EZ_EDITORFRAMEWORK_DLL ezQtBlackboardConditionWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  ezQtBlackboardConditionWidget();
  virtual ~ezQtBlackboardConditionWidget();

protected Q_SLOTS:
  void onOperatorChanged(int iIndex);
  void onBeginTemporary();
  void onEndTemporary();
  void onValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

  void SetEntryName(ezStringView sName);
  void BroadcastCurrentValue();

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  ezQtDynamicStringEnumMenuButton* m_pEntryButton = nullptr;
  QComboBox* m_pOperator = nullptr;
  ezQtDoubleSpinBox* m_pComparisonValue = nullptr;
  ezBlackboardCondition m_CurrentValue;
};
