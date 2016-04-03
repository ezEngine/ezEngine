#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Types/Variant.h>

class ezManipulatorAttribute;
class ezDocumentObject;
struct ezDocumentObjectPropertyEvent;

class EZ_EDITORFRAMEWORK_DLL ezManipulatorAdapter
{
public:
  ezManipulatorAdapter();
  virtual ~ezManipulatorAdapter();

  void SetManipulator(const ezManipulatorAttribute* pAttribute, const ezDocumentObject* pObject);

private:
  void DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

protected:
  virtual void Finalize() = 0;
  virtual void Update() = 0;
  virtual ezTransform GetObjectTransform() const;

  void BeginTemporaryInteraction();
  void EndTemporaryInteraction();
  void CancelTemporayInteraction();
  void ChangeProperties(const char* szProperty1, const ezVariant& value1,
                        const char* szProperty2 = nullptr, const ezVariant& value2 = ezVariant(),
                        const char* szProperty3 = nullptr, const ezVariant& value3 = ezVariant(),
                        const char* szProperty4 = nullptr, const ezVariant& value4 = ezVariant());

  const ezManipulatorAttribute* m_pManipulatorAttr;
  const ezDocumentObject* m_pObject;
};