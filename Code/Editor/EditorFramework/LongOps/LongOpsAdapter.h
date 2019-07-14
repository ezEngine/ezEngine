#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashSet.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct ezDocumentObjectStructureEvent;
struct ezPhantomRttiManagerEvent;
class ezRTTI;

/// \brief This singleton lives in the editor process and monitors all ezSceneDocument's for components with the ezLongOpAttribute.
///
/// All such components will be automatically registered in the ezLongOpControllerManager, such that their functionality
/// is exposed to the user.
///
/// Since this class adapts the components with the ezLongOpAttribute to the ezLongOpControllerManager, it does not have any public
/// functionality.
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
