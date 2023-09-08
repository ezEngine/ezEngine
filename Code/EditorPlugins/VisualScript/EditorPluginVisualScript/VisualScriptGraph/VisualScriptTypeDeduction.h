#pragma once

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class ezVisualScriptPin;

class ezVisualScriptTypeDeduction
{
public:
  static ezVisualScriptDataType::Enum DeductFromNodeDataType(const ezVisualScriptPin& pin);
  static ezVisualScriptDataType::Enum DeductFromTypeProperty(const ezVisualScriptPin& pin);

  static ezVisualScriptDataType::Enum DeductFromAllInputPins(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);
  static ezVisualScriptDataType::Enum DeductFromVariableNameProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);
  static ezVisualScriptDataType::Enum DeductFromScriptDataTypeProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);
  static ezVisualScriptDataType::Enum DeductFromPropertyProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);

  static const ezRTTI* GetReflectedType(const ezDocumentObject* pObject);
  static const ezAbstractProperty* GetReflectedProperty(const ezDocumentObject* pObject);
};
