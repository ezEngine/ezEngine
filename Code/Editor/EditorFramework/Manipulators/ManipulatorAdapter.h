#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Document/Document.h>

class ezManipulatorAttribute;
class ezDocumentObject;
struct ezDocumentObjectPropertyEvent;
struct ezQtDocumentWindowEvent;
class ezObjectAccessorBase;
class ezGridSettingsMsgToEngine;

class EZ_EDITORFRAMEWORK_DLL ezManipulatorAdapter
{
public:
  ezManipulatorAdapter();
  virtual ~ezManipulatorAdapter();

  void SetManipulator(const ezManipulatorAttribute* pAttribute, const ezDocumentObject* pObject);

  virtual void QueryGridSettings(ezGridSettingsMsgToEngine& outGridSettings) {}

private:
  void DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);

protected:
  virtual ezTransform GetObjectTransform() const;
  ezObjectAccessorBase* GetObjectAccessor() const;
  const ezAbstractProperty* GetProperty(const char* szProperty) const;

  virtual void Finalize() = 0;
  virtual void Update() = 0;
  virtual void UpdateGizmoTransform() = 0;

  void BeginTemporaryInteraction();
  void EndTemporaryInteraction();
  void CancelTemporayInteraction();
  void ChangeProperties(const char* szProperty1, ezVariant value1,
                        const char* szProperty2 = nullptr, ezVariant value2 = ezVariant(),
                        const char* szProperty3 = nullptr, ezVariant value3 = ezVariant(),
                        const char* szProperty4 = nullptr, ezVariant value4 = ezVariant(),
                        const char* szProperty5 = nullptr, ezVariant value5 = ezVariant(),
                        const char* szProperty6 = nullptr, ezVariant value6 = ezVariant());

  bool m_bManipulatorIsVisible;
  const ezManipulatorAttribute* m_pManipulatorAttr;
  const ezDocumentObject* m_pObject;

  void ClampProperty(const char* szProperty, ezVariant& value) const;
};
