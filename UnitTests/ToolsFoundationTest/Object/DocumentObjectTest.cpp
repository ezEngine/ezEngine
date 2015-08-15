#include <PCH.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

EZ_CREATE_SIMPLE_TEST_GROUP(DocumentObjectManager);

EZ_CREATE_SIMPLE_TEST(DocumentObjectManager, DocumentObjectManager)
{
  ezTestDocumentObjectManager manager;
  ezDocumentObjectBase* pObject = nullptr;
  ezDocumentObjectBase* pChildObject = nullptr;
  ezDocumentObjectBase* pChildren[4] = { nullptr };
  ezDocumentObjectBase* pSubElementObject[4] = { nullptr };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DocumentObject")
  {
    EZ_TEST_BOOL(manager.CanAdd(ezObjectTest::GetStaticRTTI(), nullptr, "", 0));
    pObject = manager.CreateObject(ezObjectTest::GetStaticRTTI());
    manager.AddObject(pObject, nullptr, "", 0);

    const char* szProperty = "SubObjectSet";
    EZ_TEST_BOOL(!manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, szProperty, 0));
    EZ_TEST_BOOL(manager.CanAdd(ezObjectTest::GetStaticRTTI(), pObject, szProperty, 0));
    pChildObject = manager.CreateObject(ezObjectTest::GetStaticRTTI());
    manager.AddObject(pChildObject, pObject, "SubObjectSet", 0);
    EZ_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 1);

    EZ_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0));
    EZ_TEST_BOOL(manager.CanAdd(ezReflectedClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0));
    EZ_TEST_BOOL(manager.CanAdd(ezObjectTest::GetStaticRTTI(), pObject, "ClassPtrArray", 0));

    for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(pChildren); i++)
    {
      EZ_TEST_BOOL(manager.CanAdd(ezObjectTest::GetStaticRTTI(), pChildObject, szProperty, i));
      pChildren[i] = manager.CreateObject(ezObjectTest::GetStaticRTTI());
      manager.AddObject(pChildren[i], pChildObject, szProperty, i);
      EZ_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), i + 1);
    }
    EZ_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), 4);

    EZ_TEST_BOOL_MSG(!manager.CanMove(pObject, pChildObject, szProperty, 0), "Can't move to own child");
    EZ_TEST_BOOL_MSG(!manager.CanMove(pChildren[1], pChildObject, szProperty, 1), "Can't move before onself");
    EZ_TEST_BOOL_MSG(!manager.CanMove(pChildren[1], pChildObject, szProperty, 2), "Can't move after oneself");
    EZ_TEST_BOOL_MSG(!manager.CanMove(pChildren[1], pChildren[1], szProperty, 0), "Can't move into yourself");


  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DocumentSubElementObject")
  {
    const char* szProperty = "ClassArray";
    for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(pSubElementObject); i++)
    {
      EZ_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, szProperty, i));
      pSubElementObject[i] = manager.CreateObject(OuterClass::GetStaticRTTI());
      manager.AddObject(pSubElementObject[i], pObject, szProperty, i);
      EZ_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), i + 1);
    }
    
    EZ_TEST_BOOL(manager.CanRemove(pSubElementObject[0]));
    manager.RemoveObject(pSubElementObject[0]);
    manager.DestroyObject(pSubElementObject[0]);
    pSubElementObject[0] = nullptr;
    EZ_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 3);

    ezVariant value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[1]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 2);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[3]->GetGuid());

    EZ_TEST_BOOL(manager.CanMove(pSubElementObject[1], pObject, szProperty, 2));
    manager.MoveObject(pSubElementObject[1], pObject, szProperty, 2);
    EZ_TEST_BOOL(manager.CanMove(pSubElementObject[3], pObject, szProperty, 0));
    manager.MoveObject(pSubElementObject[3], pObject, szProperty, 0);

    value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[3]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 2);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[1]->GetGuid());

    EZ_TEST_BOOL(manager.CanRemove(pSubElementObject[3]));
    manager.RemoveObject(pSubElementObject[3]);
    manager.DestroyObject(pSubElementObject[3]);
    pSubElementObject[3] = nullptr;
    EZ_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 2);

    value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[1]->GetGuid());

    EZ_TEST_BOOL(manager.CanMove(pSubElementObject[1], pChildObject, szProperty, 0));
    manager.MoveObject(pSubElementObject[1], pChildObject, szProperty, 0);
    EZ_TEST_BOOL(manager.CanMove(pSubElementObject[2], pChildObject, szProperty, 0));
    manager.MoveObject(pSubElementObject[2], pChildObject, szProperty, 0);

    EZ_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 0);
    EZ_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), 2);

    value = pChildObject->GetTypeAccessor().GetValue(szProperty, 0);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[2]->GetGuid());
    value = pChildObject->GetTypeAccessor().GetValue(szProperty, 1);
    EZ_TEST_BOOL(value.IsA<ezUuid>() && value.Get<ezUuid>() == pSubElementObject[1]->GetGuid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DocumentSubObject")
  {
    // Temp object to limit accessor scope.
    ezDocumentSubObject* pSubObject = EZ_DEFAULT_NEW(ezDocumentSubObject, OuterClass::GetStaticRTTI());
    pSubObject->SetObject(pObject, "MemberClass");

    EZ_DEFAULT_DELETE(pSubObject);
  }

  manager.DestroyAllObjects();
}

