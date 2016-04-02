#pragma once

#include <EditorFramework/Plugin.h>

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

  const ezManipulatorAttribute* m_pManipulatorAttr;
  const ezDocumentObject* m_pObject;
};