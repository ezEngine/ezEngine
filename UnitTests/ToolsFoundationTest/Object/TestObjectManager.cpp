#include <PCH.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>

ezTestDocumentObjectManager::ezTestDocumentObjectManager()
{
  m_pMetaRtti = ezReflectedClass::GetStaticRTTI();
}

ezTestDocumentObjectManager::~ezTestDocumentObjectManager()
{
}

void ezTestDocumentObjectManager::GetCreateableTypes(ezHybridArray<ezRTTI*, 32>& Types) const
{
}

ezDocumentObjectBase* ezTestDocumentObjectManager::InternalCreateObject(const ezRTTI* pRtti)
{
  auto pObject = EZ_DEFAULT_NEW(ezDocumentObject, m_pMetaRtti, pRtti);
  return pObject;
}
