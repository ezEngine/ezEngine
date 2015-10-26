#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Containers/HybridArray.h>


class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectMirror
{
public:
  ezDocumentObjectMirror();
  ~ezDocumentObjectMirror();

  void Init(const ezDocumentObjectManager* pManager);
  void DeInit();

  void TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  void* GetNativeObjectPointer(const ezDocumentObject* pObject);
  const void* GetNativeObjectPointer(const ezDocumentObject* pObject) const;

private:
  bool IsRootObject(const ezDocumentObject* pParent);
  bool IsHeapAllocated(const ezDocumentObject* pParent, const char* szParentProperty);
  ezUuid FindRootOpObject(const ezDocumentObject* pObject, ezHybridArray<const ezDocumentObject*, 8>& path);

private:
  ezRttiConverterContext m_Context;
  const ezDocumentObjectManager* m_pManager;
};