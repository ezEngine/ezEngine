#pragma once

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class ezVisualScriptVariableAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptVariableAttribute, ezTypeWidgetAttribute);
};

//////////////////////////////////////////////////////////////////////////

class ezQtVisualScriptVariableWidget : public ezQtVariantPropertyWidget
{
  Q_OBJECT;

public:
  ezQtVisualScriptVariableWidget();
  virtual ~ezQtVisualScriptVariableWidget();

protected:
  virtual ezResult GetVariantTypeDisplayName(ezVariantType::Enum type, ezStringBuilder& out_sName) const override;
};
