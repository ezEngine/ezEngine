#include <PCH.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezDocumentObjectVisitor::ezDocumentObjectVisitor(const ezDocumentObjectManager* pManager, const char* szChildrenProperty /*= "Children"*/, const char* szRootProperty /*= "Children"*/) : m_pManager(pManager), m_sChildrenProperty(szChildrenProperty), m_sRootProperty(szRootProperty)
{
  const ezAbstractProperty* pRootProp = m_pManager->GetRootObject()->GetType()->FindPropertyByName(szRootProperty);
  EZ_ASSERT_DEV(pRootProp, "Given root property '{0}' does not exist on root object", szRootProperty);
  EZ_ASSERT_DEV(pRootProp->GetCategory() == ezPropertyCategory::Set || pRootProp->GetCategory() == ezPropertyCategory::Array, "Traverser only works on arrays and sets.");

  //const ezAbstractProperty* pChildProp = pRootProp->GetSpecificType()->FindPropertyByName(szChildrenProperty);
  //EZ_ASSERT_DEV(pChildProp, "Given child property '{0}' does not exist", szChildrenProperty);
  //EZ_ASSERT_DEV(pChildProp->GetCategory() == ezPropertyCategory::Set || pRootProp->GetCategory() == ezPropertyCategory::Array, "Traverser only works on arrays and sets.");
}

void ezDocumentObjectVisitor::Visit(const ezDocumentObject* pObject, bool bVisitStart, VisitorFunction function)
{
  const char* szProperty = m_sChildrenProperty;
  if (pObject == nullptr || pObject == m_pManager->GetRootObject())
  {
    pObject = m_pManager->GetRootObject();
    szProperty = m_sRootProperty;
  }

  if (!bVisitStart || function(pObject))
  {
    TraverseChildren(pObject, szProperty, function);
  }
}

void ezDocumentObjectVisitor::TraverseChildren(const ezDocumentObject* pObject, const char* szProperty, VisitorFunction& function)
{
  const ezInt32 iChildren = pObject->GetTypeAccessor().GetCount(szProperty);
  for (ezInt32 i = 0; i < iChildren; i++)
  {
    ezVariant obj = pObject->GetTypeAccessor().GetValue(szProperty, i);
    EZ_ASSERT_DEBUG(obj.IsValid() && obj.IsA<ezUuid>(), "null obj found during traversal.");
    const ezDocumentObject* pChild = m_pManager->GetObject(obj.Get<ezUuid>());
    if (function(pChild))
    {
      TraverseChildren(pChild, m_sChildrenProperty, function);
    }
  }
}
