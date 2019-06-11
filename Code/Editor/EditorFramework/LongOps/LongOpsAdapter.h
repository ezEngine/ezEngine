#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashSet.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct ezDocumentObjectStructureEvent;
struct ezPhantomRttiManagerEvent;
class ezRTTI;

class ezLongOpsAdapter
{
  EZ_DECLARE_SINGLETON(ezLongOpsAdapter);

public:
  ezLongOpsAdapter();
  ~ezLongOpsAdapter();

private:
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e);
  void CheckAllTypes();
  void ObjectAdded(const ezDocumentObject* pObject);
  void ObjectRemoved(const ezDocumentObject* pObject);

  ezHashSet<const ezRTTI*> m_TypesWithLongOps;
};
