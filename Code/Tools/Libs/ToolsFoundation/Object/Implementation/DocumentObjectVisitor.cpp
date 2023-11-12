#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

ezDocumentObjectVisitor::ezDocumentObjectVisitor(
  const ezDocumentObjectManager* pManager, ezStringView sChildrenProperty /*= "Children"*/, ezStringView sRootProperty /*= "Children"*/)
  : m_pManager(pManager)
  , m_sChildrenProperty(sChildrenProperty)
  , m_sRootProperty(sRootProperty)
{
  const ezAbstractProperty* pRootProp = m_pManager->GetRootObject()->GetType()->FindPropertyByName(sRootProperty);
  EZ_ASSERT_DEV(pRootProp, "Given root property '{0}' does not exist on root object", sRootProperty);
  EZ_ASSERT_DEV(pRootProp->GetCategory() == ezPropertyCategory::Set || pRootProp->GetCategory() == ezPropertyCategory::Array,
    "Traverser only works on arrays and sets.");

  // const ezAbstractProperty* pChildProp = pRootProp->GetSpecificType()->FindPropertyByName(szChildrenProperty);
  // EZ_ASSERT_DEV(pChildProp, "Given child property '{0}' does not exist", szChildrenProperty);
  // EZ_ASSERT_DEV(pChildProp->GetCategory() == ezPropertyCategory::Set || pRootProp->GetCategory() == ezPropertyCategory::Array, "Traverser
  // only works on arrays and sets.");
}

void ezDocumentObjectVisitor::Visit(const ezDocumentObject* pObject, bool bVisitStart, VisitorFunction function)
{
  ezStringView sProperty = m_sChildrenProperty;
  if (pObject == nullptr || pObject == m_pManager->GetRootObject())
  {
    pObject = m_pManager->GetRootObject();
    sProperty = m_sRootProperty;
  }

  if (!bVisitStart || function(pObject))
  {
    TraverseChildren(pObject, sProperty, function);
  }
}

void ezDocumentObjectVisitor::TraverseChildren(const ezDocumentObject* pObject, ezStringView sProperty, VisitorFunction& function)
{
  const ezInt32 iChildren = pObject->GetTypeAccessor().GetCount(sProperty);
  for (ezInt32 i = 0; i < iChildren; i++)
  {
    ezVariant obj = pObject->GetTypeAccessor().GetValue(sProperty, i);
    EZ_ASSERT_DEBUG(obj.IsValid() && obj.IsA<ezUuid>(), "null obj found during traversal.");
    const ezDocumentObject* pChild = m_pManager->GetObject(obj.Get<ezUuid>());
    if (function(pChild))
    {
      TraverseChildren(pChild, m_sChildrenProperty, function);
    }
  }
}
