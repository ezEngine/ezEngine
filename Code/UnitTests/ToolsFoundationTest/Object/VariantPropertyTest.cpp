#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

static ezHybridArray<ezDocumentObjectPropertyEvent, 2> s_Changes;
void TestPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  s_Changes.PushBack(e);
}

void TestArray(ezVariantSubAccessor& ref_accessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezDelegate<ezVariant()>& getNativeValue)
{
  auto VerifyChange = [&]()
  {
    // Any operation should collapse to the ezVariant being set as a whole.
    EZ_TEST_INT(s_Changes.GetCount(), 1);
    EZ_TEST_BOOL(s_Changes[0].m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet);
    EZ_TEST_BOOL(s_Changes[0].m_pObject == pObject);
    EZ_TEST_BOOL(s_Changes[0].m_sProperty == pProp->GetPropertyName());
    s_Changes.Clear();
  };

  s_Changes.Clear();
  ref_accessor.StartTransaction("Insert Element");
  ezInt32 iCount = 0;
  ezVariant value = ezColor(1, 2, 3);
  EZ_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  EZ_TEST_INT(iCount, 0);
  EZ_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value, 0));
  EZ_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  EZ_TEST_INT(iCount, 1);
  EZ_TEST_BOOL(getNativeValue()[0] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();
  s_Changes.Clear();

  ezVariant outValue;
  EZ_TEST_STATUS(ref_accessor.GetValue(pObject, pProp, outValue, 0));
  EZ_TEST_BOOL(value == outValue);

  ref_accessor.StartTransaction("Set Element");
  value = ezVariantDictionary();
  EZ_TEST_STATUS(ref_accessor.SetValue(pObject, pProp, value, 0));
  EZ_TEST_BOOL(getNativeValue()[0] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Insert Element");
  ezVariant value2 = "Test";
  EZ_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value2, 1));
  EZ_TEST_BOOL(getNativeValue()[0] == value);
  EZ_TEST_BOOL(getNativeValue()[1] == value2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Move Element");
  EZ_TEST_STATUS(ref_accessor.MoveValue(pObject, pProp, 1, 0));
  EZ_TEST_BOOL(getNativeValue()[0] == value2);
  EZ_TEST_BOOL(getNativeValue()[1] == value);
  EZ_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  EZ_TEST_INT(iCount, 2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Remove Element");
  EZ_TEST_STATUS(ref_accessor.RemoveValue(pObject, pProp, 0));
  EZ_TEST_BOOL(getNativeValue()[0] == value);
  EZ_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  EZ_TEST_INT(iCount, 1);
  ref_accessor.FinishTransaction();
  VerifyChange();
}

void TestDictionary(ezVariantSubAccessor& ref_accessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezDelegate<ezVariant()>& getNativeValue)
{
  auto VerifyChange = [&]()
  {
    // Any operation should collapse to the ezVariant being set as a whole.
    EZ_TEST_INT(s_Changes.GetCount(), 1);
    EZ_TEST_BOOL(s_Changes[0].m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet);
    EZ_TEST_BOOL(s_Changes[0].m_pObject == pObject);
    EZ_TEST_BOOL(s_Changes[0].m_sProperty == pProp->GetPropertyName());
    s_Changes.Clear();
  };

  s_Changes.Clear();
  ref_accessor.StartTransaction("Insert Element");
  ezInt32 iCount = 0;
  ezVariant value = ezColor(1, 2, 3);
  EZ_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  EZ_TEST_INT(iCount, 0);
  EZ_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value, "A"));
  EZ_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  EZ_TEST_INT(iCount, 1);
  EZ_TEST_BOOL(getNativeValue()["A"] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();
  s_Changes.Clear();

  ezVariant outValue;
  EZ_TEST_STATUS(ref_accessor.GetValue(pObject, pProp, outValue, "A"));
  EZ_TEST_BOOL(value == outValue);

  ref_accessor.StartTransaction("Set Element");
  value = 42u;
  EZ_TEST_STATUS(ref_accessor.SetValue(pObject, pProp, value, "A"));
  EZ_TEST_BOOL(getNativeValue()["A"] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Insert Element");
  ezVariant value2 = ezVariantArray();
  EZ_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value2, "B"));
  EZ_TEST_BOOL(getNativeValue()["A"] == value);
  EZ_TEST_BOOL(getNativeValue()["B"] == value2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Remove Element");
  EZ_TEST_STATUS(ref_accessor.RemoveValue(pObject, pProp, "A"));
  EZ_TEST_BOOL(getNativeValue()["B"] == value2);
  EZ_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  EZ_TEST_INT(iCount, 1);
  ref_accessor.FinishTransaction();
  VerifyChange();
}

EZ_CREATE_SIMPLE_TEST(DocumentObject, VariantPropertyTest)
{
  EZ_SCOPE_EXIT(s_Changes.Clear(); s_Changes.Compact(););
  ezTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  ezObjectAccessorBase* pAccessor = doc.GetObjectAccessor();
  const ezDocumentObject* pObject = nullptr;
  const ezVariantTestStruct* pNative = nullptr;
  doc.GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&TestPropertyEventHandler));
  EZ_SCOPE_EXIT(doc.GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&TestPropertyEventHandler)));

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CreateObject")
  {
    ezUuid objGuid;
    pAccessor->StartTransaction("Add Object");
    EZ_TEST_STATUS(pAccessor->AddObject(nullptr, (const ezAbstractProperty*)nullptr, -1, ezGetStaticRTTI<ezVariantTestStruct>(), objGuid));
    pAccessor->FinishTransaction();
    pObject = pAccessor->GetObject(objGuid);
    pNative = static_cast<ezVariantTestStruct*>(doc.m_ObjectMirror.GetNativeObjectPointer(pObject));
  }

  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName("Variant");
  const ezAbstractProperty* pPropArray = pObject->GetType()->FindPropertyByName("VariantArray");
  const ezAbstractProperty* pPropDict = pObject->GetType()->FindPropertyByName("VariantDictionary");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TestVariant")
  {
    pAccessor->StartTransaction("Set as Array");
    EZ_TEST_STATUS(pAccessor->SetValue(pObject, pProp, ezVariantArray()));
    pAccessor->FinishTransaction();
    s_Changes.Clear();

    ezVariantSubAccessor accessor(pAccessor, pProp);
    ezMap<const ezDocumentObject*, ezVariant> subItemMap;
    subItemMap.Insert(pObject, ezVariant());
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pProp, [&]()
      { return pNative->m_Variant; });
    // What remains is an ezVariantDictionary at index 0 that we can recurse into.
    {
      ezVariantSubAccessor accessor2(&accessor, pProp);
      ezMap<const ezDocumentObject*, ezVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pProp, [&]()
        { return pNative->m_Variant[0]; });
    }
    pAccessor->StartTransaction("Set as Dict");
    EZ_TEST_STATUS(pAccessor->SetValue(pObject, pProp, ezVariantDictionary()));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    TestDictionary(accessor, pObject, pProp, [&]()
      { return pNative->m_Variant; });
    // What remains is an ezVariantArray at index "B" that we can recurse into.
    {
      ezVariantSubAccessor accessor2(&accessor, pProp);
      ezMap<const ezDocumentObject*, ezVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pProp, [&]()
        { return pNative->m_Variant["B"]; });
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TestVariantArray")
  {
    pAccessor->StartTransaction("Insert Array");
    EZ_TEST_STATUS(pAccessor->InsertValue(pObject, pPropArray, ezVariantArray(), 0));
    pAccessor->FinishTransaction();

    ezVariantSubAccessor accessor(pAccessor, pPropArray);
    ezMap<const ezDocumentObject*, ezVariant> subItemMap;
    subItemMap.Insert(pObject, 0);
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pPropArray, [&]()
      { return pNative->m_VariantArray[0]; });
    // What remains is an ezVariantDictionary at index 0 that we can recurse into.
    {
      ezVariantSubAccessor accessor2(&accessor, pPropArray);
      ezMap<const ezDocumentObject*, ezVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pPropArray, [&]()
        { return pNative->m_VariantArray[0][0]; });
    }
    pAccessor->StartTransaction("Insert Dictionary");
    EZ_TEST_STATUS(pAccessor->InsertValue(pObject, pPropArray, ezVariantDictionary(), 1));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    subItemMap.Insert(pObject, 1);
    accessor.SetSubItems(subItemMap);
    TestDictionary(accessor, pObject, pPropArray, [&]()
      { return pNative->m_VariantArray[1]; });
    // What remains is an ezVariantArray at index "B" that we can recurse into.
    {
      ezVariantSubAccessor accessor2(&accessor, pPropArray);
      ezMap<const ezDocumentObject*, ezVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pPropArray, [&]()
        { return pNative->m_VariantArray[1]["B"]; });
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TestVariantDictionary")
  {
    pAccessor->StartTransaction("Insert Array");
    EZ_TEST_STATUS(pAccessor->InsertValue(pObject, pPropDict, ezVariantArray(), "AAA"));
    pAccessor->FinishTransaction();

    ezVariantSubAccessor accessor(pAccessor, pPropDict);
    ezMap<const ezDocumentObject*, ezVariant> subItemMap;
    subItemMap.Insert(pObject, "AAA");
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pPropDict, [&]()
      { return *pNative->m_VariantDictionary.GetValue("AAA"); });
    // What remains is an ezVariantDictionary at index 0 that we can recurse into.
    {
      ezVariantSubAccessor accessor2(&accessor, pPropDict);
      ezMap<const ezDocumentObject*, ezVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pPropDict, [&]()
        { return (*pNative->m_VariantDictionary.GetValue("AAA"))[0]; });
    }
    pAccessor->StartTransaction("Insert Dictionary");
    EZ_TEST_STATUS(pAccessor->InsertValue(pObject, pPropDict, ezVariantDictionary(), "BBB"));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    subItemMap.Insert(pObject, "BBB");
    accessor.SetSubItems(subItemMap);
    TestDictionary(accessor, pObject, pPropDict, [&]()
      { return *pNative->m_VariantDictionary.GetValue("BBB"); });
    // What remains is an ezVariantArray at index "B" that we can recurse into.
    {
      ezVariantSubAccessor accessor2(&accessor, pPropDict);
      ezMap<const ezDocumentObject*, ezVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pPropDict, [&]()
        { return (*pNative->m_VariantDictionary.GetValue("BBB"))["B"]; });
    }
  }
}
