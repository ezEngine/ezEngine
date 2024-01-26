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

//////////////////////////////////////////////////////////////////////////

struct ezVisualScriptExpressionDataType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Int = static_cast<ezUInt8>(ezProcessingStream::DataType::Int),
    Float = static_cast<ezUInt8>(ezProcessingStream::DataType::Float),
    Vector3 = static_cast<ezUInt8>(ezProcessingStream::DataType::Float3),
    Color = static_cast<ezUInt8>(ezProcessingStream::DataType::Float4),

    Default = Float
  };

  static ezVisualScriptDataType::Enum GetVisualScriptDataType(Enum dataType);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptExpressionDataType);

struct ezVisualScriptExpressionVariable
{
  ezHashedString m_sName;
  ezEnum<ezVisualScriptExpressionDataType> m_Type;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptExpressionVariable);
