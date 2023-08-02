#pragma once

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct ezVisualScriptVariable
{
  ezHashedString m_sName;
  ezVariant m_DefaultValue;
  bool m_bExpose = false;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptVariable);

//////////////////////////////////////////////////////////////////////////

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
