#pragma once

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class ezVisualScriptPin;

class ezVisualScriptTypeDeduction
{
public:
  static ezVisualScriptDataType::Enum DeductFromNodeDataType(const ezVisualScriptPin& pin);
  static ezVisualScriptDataType::Enum DeductFromTypeProperty(const ezVisualScriptPin& pin);
  static ezVisualScriptDataType::Enum DeductFromExpressionInput(const ezVisualScriptPin& pin);
  static ezVisualScriptDataType::Enum DeductFromExpressionOutput(const ezVisualScriptPin& pin);

  static ezVisualScriptDataType::Enum DeductFromAllInputPins(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);
  static ezVisualScriptDataType::Enum DeductFromVariableNameProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);
  static ezVisualScriptDataType::Enum DeductFromScriptDataTypeProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);
  static ezVisualScriptDataType::Enum DeductFromPropertyProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);
  static ezVisualScriptDataType::Enum DeductDummy(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);

  static const ezRTTI* GetReflectedType(const ezDocumentObject* pObject);
  static const ezAbstractProperty* GetReflectedProperty(const ezDocumentObject* pObject);

private:
  static ezVisualScriptDataType::Enum DeductFromExpressionVariable(const ezVisualScriptPin& pin, ezStringView sPropertyName);
};
