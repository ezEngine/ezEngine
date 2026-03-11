#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

EZ_CREATE_SIMPLE_TEST(DocumentObject, CommandHistory)
{
  ezTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  ezObjectAccessorBase* pAccessor = doc.GetObjectAccessor();

  auto CreateObject = [&doc, &pAccessor](const ezRTTI* pType) -> const ezDocumentObject*
  {
    ezUuid objGuid;
    pAccessor->StartTransaction("Add Object");
    EZ_TEST_STATUS(pAccessor->AddObject(nullptr, (const ezAbstractProperty*)nullptr, -1, pType, objGuid));
    pAccessor->FinishTransaction();
    return pAccessor->GetObject(objGuid);
  };

  auto StoreOriginalState = [&doc](ezAbstractObjectGraph& ref_graph, const ezDocumentObject* pRoot)
  {
    ezDocumentObjectConverterWriter writer(&ref_graph, doc.GetObjectManager(), [](const ezDocumentObject*, const ezAbstractProperty* p)
      { return p->GetAttributeByType<ezHiddenAttribute>() == nullptr; });
    ezAbstractObjectNode* pAbstractObj = writer.AddObjectToGraph(pRoot);
  };

  auto CompareAgainstOriginalState = [&doc](ezAbstractObjectGraph& ref_original, const ezDocumentObject* pRoot)
  {
    ezAbstractObjectGraph graph;
    ezDocumentObjectConverterWriter writer2(&graph, doc.GetObjectManager(), [](const ezDocumentObject*, const ezAbstractProperty* p)
      { return p->GetAttributeByType<ezHiddenAttribute>() == nullptr; });
    ezAbstractObjectNode* pAbstractObj2 = writer2.AddObjectToGraph(pRoot);

    ezDeque<ezAbstractGraphDiffOperation> diff;
    graph.CreateDiffWithBaseGraph(ref_original, diff);
    EZ_TEST_BOOL(diff.GetCount() == 0);
  };

  const ezDocumentObject* pRoot = CreateObject(ezGetStaticRTTI<ezMirrorTest>());

  ezUuid mathGuid = pAccessor->GetByName<ezUuid>(pRoot, "Math");
  ezUuid objectGuid = pAccessor->GetByName<ezUuid>(pRoot, "Object");

  const ezDocumentObject* pMath = pAccessor->GetObject(mathGuid);
  const ezDocumentObject* pObjectTest = pAccessor->GetObject(objectGuid);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetValue")
  {

    EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), 1);

    auto TestSetValue = [&](const ezDocumentObject* pObject, const char* szProperty, ezVariant value)
    {
      ezAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      ezUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();

      pAccessor->StartTransaction("SetValue");
      EZ_TEST_STATUS(pAccessor->SetValueByName(pObject, szProperty, value));
      pAccessor->FinishTransaction();
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      ezVariant newValue;
      EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue));
      EZ_TEST_BOOL(newValue == value);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Undo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      CompareAgainstOriginalState(graph, pObject);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Redo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue));
      EZ_TEST_BOOL(newValue == value);
    };

    // Math
    TestSetValue(pMath, "Vec2", ezVec2(1, 2));
    TestSetValue(pMath, "Vec3", ezVec3(1, 2, 3));
    TestSetValue(pMath, "Vec4", ezVec4(1, 2, 3, 4));
    TestSetValue(pMath, "Vec2I", ezVec2I32(1, 2));
    TestSetValue(pMath, "Vec3I", ezVec3I32(1, 2, 3));
    TestSetValue(pMath, "Vec4I", ezVec4I32(1, 2, 3, 4));
    ezQuat qValue;
    qValue = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(30), ezAngle::MakeFromDegree(30), ezAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Quat", qValue);
    ezMat3 mValue;
    mValue = ezMat3::MakeRotationX(ezAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Mat3", mValue);
    ezMat4 mValue2;
    mValue2.SetIdentity();
    mValue2 = ezMat4::MakeRotationX(ezAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Mat4", mValue2);

    // Integer
    const ezDocumentObject* pInteger = CreateObject(ezGetStaticRTTI<ezIntegerStruct>());
    TestSetValue(pInteger, "Int8", ezInt8(-5));
    TestSetValue(pInteger, "UInt8", ezUInt8(5));
    TestSetValue(pInteger, "Int16", ezInt16(-5));
    TestSetValue(pInteger, "UInt16", ezUInt16(5));
    TestSetValue(pInteger, "Int32", ezInt32(-5));
    TestSetValue(pInteger, "UInt32", ezUInt32(5));
    TestSetValue(pInteger, "Int64", ezInt64(-5));
    TestSetValue(pInteger, "UInt64", ezUInt64(5));

    // Test automatic type conversions
    TestSetValue(pInteger, "Int8", ezInt16(-5));
    TestSetValue(pInteger, "Int8", ezInt32(-5));
    TestSetValue(pInteger, "Int8", ezInt64(-5));
    TestSetValue(pInteger, "Int8", float(-5));
    TestSetValue(pInteger, "Int8", ezUInt8(5));

    TestSetValue(pInteger, "Int64", ezInt32(-5));
    TestSetValue(pInteger, "Int64", ezInt16(-5));
    TestSetValue(pInteger, "Int64", ezInt8(-5));
    TestSetValue(pInteger, "Int64", float(-5));
    TestSetValue(pInteger, "Int64", ezUInt8(5));

    TestSetValue(pInteger, "UInt64", ezUInt32(5));
    TestSetValue(pInteger, "UInt64", ezUInt16(5));
    TestSetValue(pInteger, "UInt64", ezUInt8(5));
    TestSetValue(pInteger, "UInt64", float(5));
    TestSetValue(pInteger, "UInt64", ezInt8(5));

    // Float
    const ezDocumentObject* pFloat = CreateObject(ezGetStaticRTTI<ezFloatStruct>());
    TestSetValue(pFloat, "Float", -5.0f);
    TestSetValue(pFloat, "Double", -5.0);
    TestSetValue(pFloat, "Time", ezTime::MakeFromMinutes(3.0f));
    TestSetValue(pFloat, "Angle", ezAngle::MakeFromDegree(45.0f));

    TestSetValue(pFloat, "Float", 5.0);
    TestSetValue(pFloat, "Float", ezInt8(-5));
    TestSetValue(pFloat, "Float", ezUInt8(5));

    // Misc PODs
    const ezDocumentObject* pPOD = CreateObject(ezGetStaticRTTI<ezPODClass>());
    TestSetValue(pPOD, "Bool", true);
    TestSetValue(pPOD, "Bool", false);
    TestSetValue(pPOD, "Color", ezColor(1.0f, 2.0f, 3.0f, 4.0f));
    TestSetValue(pPOD, "ColorUB", ezColorGammaUB(200, 100, 255));
    TestSetValue(pPOD, "String", "Test");
    ezVarianceTypeAngle customFloat;
    customFloat.m_Value = ezAngle::MakeFromDegree(45.0f);
    customFloat.m_fVariance = 1.0f;
    TestSetValue(pPOD, "VarianceAngle", customFloat);

    // Enumerations
    const ezDocumentObject* pEnum = CreateObject(ezGetStaticRTTI<ezEnumerationsClass>());
    TestSetValue(pEnum, "Enum", (ezInt8)ezExampleEnum::Value2);
    TestSetValue(pEnum, "Enum", (ezInt64)ezExampleEnum::Value2);
    TestSetValue(pEnum, "Bitflags", (ezUInt8)ezExampleBitflags::Value2);
    TestSetValue(pEnum, "Bitflags", (ezInt64)ezExampleBitflags::Value2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "InsertValue")
  {
    auto TestInsertValue = [&](const ezDocumentObject* pObject, const char* szProperty, ezVariant value, ezVariant index)
    {
      ezAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const ezUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const ezInt32 iArraySize = pAccessor->GetCountByName(pObject, szProperty);

      pAccessor->StartTransaction("InsertValue");
      EZ_TEST_STATUS(pAccessor->InsertValueByName(pObject, szProperty, value, index));
      pAccessor->FinishTransaction();
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize + 1);
      ezVariant newValue;
      EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, index));
      EZ_TEST_BOOL(newValue == value);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Undo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Redo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize + 1);
      EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, index));
      EZ_TEST_BOOL(newValue == value);
    };

    TestInsertValue(pObjectTest, "StandardTypeArray", double(0), 0);
    TestInsertValue(pObjectTest, "StandardTypeArray", double(2), 1);
    TestInsertValue(pObjectTest, "StandardTypeArray", double(1), 1);

    TestInsertValue(pObjectTest, "StandardTypeSet", "A", 0);
    TestInsertValue(pObjectTest, "StandardTypeSet", "C", 1);
    TestInsertValue(pObjectTest, "StandardTypeSet", "B", 1);

    TestInsertValue(pObjectTest, "StandardTypeMap", double(0), "A");
    TestInsertValue(pObjectTest, "StandardTypeMap", double(2), "C");
    TestInsertValue(pObjectTest, "StandardTypeMap", double(1), "B");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MoveValue")
  {
    auto TestMoveValue = [&](const ezDocumentObject* pObject, const char* szProperty, ezVariant oldIndex, ezVariant newIndex, ezArrayPtr<ezVariant> expectedOutcome)
    {
      ezAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const ezUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const ezInt32 iArraySize = pAccessor->GetCountByName(pObject, szProperty);
      EZ_TEST_INT(iArraySize, expectedOutcome.GetCount());

      ezDynamicArray<ezVariant> values;
      EZ_TEST_STATUS(pAccessor->GetValuesByName(pObject, szProperty, values));
      EZ_TEST_INT(iArraySize, values.GetCount());

      pAccessor->StartTransaction("MoveValue");
      EZ_TEST_STATUS(pAccessor->MoveValueByName(pObject, szProperty, oldIndex, newIndex));
      pAccessor->FinishTransaction();
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);

      for (ezInt32 i = 0; i < iArraySize; i++)
      {
        ezVariant newValue;
        EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, i));
        EZ_TEST_BOOL(newValue == expectedOutcome[i]);
      }

      EZ_TEST_STATUS(doc.GetCommandHistory()->Undo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Redo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);

      for (ezInt32 i = 0; i < iArraySize; i++)
      {
        ezVariant newValue;
        EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, i));
        EZ_TEST_BOOL(newValue == expectedOutcome[i]);
      }
    };

    {
      ezVariant expectedValues[3] = {0, 1, 2};
      // Move first element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 0, ezArrayPtr<ezVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 1, ezArrayPtr<ezVariant>(expectedValues));
      // Move last element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 2, ezArrayPtr<ezVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 3, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      // Move first element to the end.
      ezVariant expectedValues[3] = {1, 2, 0};
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 3, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      // Move last element to the front.
      ezVariant expectedValues[3] = {0, 1, 2};
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 0, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      // Move first element to the middle
      ezVariant expectedValues[3] = {1, 0, 2};
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 2, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      // Move last element to the middle
      ezVariant expectedValues[3] = {1, 2, 0};
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 1, ezArrayPtr<ezVariant>(expectedValues));
    }

    {
      ezVariant expectedValues[3] = {"A", "B", "C"};
      // Move first element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 0, ezArrayPtr<ezVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 1, ezArrayPtr<ezVariant>(expectedValues));
      // Move last element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 2, ezArrayPtr<ezVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 3, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      // Move first element to the end.
      ezVariant expectedValues[3] = {"B", "C", "A"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 3, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      // Move last element to the front.
      ezVariant expectedValues[3] = {"A", "B", "C"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 0, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      // Move first element to the middle
      ezVariant expectedValues[3] = {"B", "A", "C"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 2, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      // Move last element to the middle
      ezVariant expectedValues[3] = {"B", "C", "A"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 1, ezArrayPtr<ezVariant>(expectedValues));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveValue")
  {
    auto TestRemoveValue = [&](const ezDocumentObject* pObject, const char* szProperty, ezVariant index, ezArrayPtr<ezVariant> expectedOutcome)
    {
      ezAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const ezUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const ezInt32 iArraySize = pAccessor->GetCountByName(pObject, szProperty);
      EZ_TEST_INT(iArraySize - 1, expectedOutcome.GetCount());

      ezDynamicArray<ezVariant> values;
      ezDynamicArray<ezVariant> keys;
      {
        EZ_TEST_STATUS(pAccessor->GetValuesByName(pObject, szProperty, values));
        EZ_TEST_INT(iArraySize, values.GetCount());

        EZ_TEST_STATUS(pAccessor->GetKeysByName(pObject, szProperty, keys));
        EZ_TEST_INT(iArraySize, keys.GetCount());
        ezUInt32 uiIndex = keys.IndexOf(index);
        keys.RemoveAtAndSwap(uiIndex);
        values.RemoveAtAndSwap(uiIndex);
        EZ_TEST_INT(iArraySize - 1, keys.GetCount());
      }

      pAccessor->StartTransaction("RemoveValue");
      EZ_TEST_STATUS(pAccessor->RemoveValueByName(pObject, szProperty, index));
      pAccessor->FinishTransaction();
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize - 1);

      if (pObject->GetType()->FindPropertyByName(szProperty)->GetCategory() == ezPropertyCategory::Map)
      {
        for (ezInt32 i = 0; i < iArraySize - 1; i++)
        {
          const ezVariant& key = keys[i];
          const ezVariant& value = values[i];
          ezVariant newValue;
          EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, key));
          EZ_TEST_BOOL(newValue == value);
          EZ_TEST_BOOL(expectedOutcome.Contains(newValue));
        }
      }
      else
      {
        for (ezInt32 i = 0; i < iArraySize - 1; i++)
        {
          ezVariant newValue;
          EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, i));
          EZ_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }

      EZ_TEST_STATUS(doc.GetCommandHistory()->Undo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Redo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize - 1);

      if (pObject->GetType()->FindPropertyByName(szProperty)->GetCategory() == ezPropertyCategory::Map)
      {
        for (ezInt32 i = 0; i < iArraySize - 1; i++)
        {
          const ezVariant& key = keys[i];
          const ezVariant& value = values[i];
          ezVariant newValue;
          EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, key));
          EZ_TEST_BOOL(newValue == value);
          EZ_TEST_BOOL(expectedOutcome.Contains(newValue));
        }
      }
      else
      {
        for (ezInt32 i = 0; i < iArraySize - 1; i++)
        {
          ezVariant newValue;
          EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, i));
          EZ_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }
    };

    // StandardTypeArray
    {
      ezVariant expectedValues[2] = {2, 0};
      TestRemoveValue(pObjectTest, "StandardTypeArray", 0, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      ezVariant expectedValues[1] = {2};
      TestRemoveValue(pObjectTest, "StandardTypeArray", 1, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeArray", 0, ezArrayPtr<ezVariant>());
    }
    // StandardTypeSet
    {
      ezVariant expectedValues[2] = {"B", "C"};
      TestRemoveValue(pObjectTest, "StandardTypeSet", 2, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      ezVariant expectedValues[1] = {"C"};
      TestRemoveValue(pObjectTest, "StandardTypeSet", 0, ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeSet", 0, ezArrayPtr<ezVariant>());
    }
    // StandardTypeMap
    {
      ezVariant expectedValues[2] = {1, 2};
      TestRemoveValue(pObjectTest, "StandardTypeMap", "A", ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      ezVariant expectedValues[1] = {1};
      TestRemoveValue(pObjectTest, "StandardTypeMap", "C", ezArrayPtr<ezVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeMap", "B", ezArrayPtr<ezVariant>());
    }
  }

  auto CreateGuid = [](const char* szType, ezInt32 iIndex) -> ezUuid
  {
    ezUuid A = ezUuid::MakeStableUuidFromString(szType);
    ezUuid B = ezUuid::MakeStableUuidFromInt(iIndex);
    A.CombineWithSeed(B);
    return A;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddObject")
  {
    auto TestAddObject = [&](const ezDocumentObject* pObject, const char* szProperty, ezVariant index, const ezRTTI* pType, ezUuid& inout_object)
    {
      ezAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const ezUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const ezInt32 iArraySize = pAccessor->GetCountByName(pObject, szProperty);

      pAccessor->StartTransaction("TestAddObject");
      EZ_TEST_STATUS(pAccessor->AddObjectByName(pObject, szProperty, index, pType, inout_object));
      pAccessor->FinishTransaction();
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize + 1);
      ezVariant newValue;
      EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, index));
      EZ_TEST_BOOL(newValue == inout_object);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Undo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Redo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize + 1);
      EZ_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, index));
      EZ_TEST_BOOL(newValue == inout_object);
    };

    ezUuid A = CreateGuid("ClassArray", 0);
    ezUuid B = CreateGuid("ClassArray", 1);
    ezUuid C = CreateGuid("ClassArray", 2);

    TestAddObject(pObjectTest, "ClassArray", 0, ezGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassArray", 1, ezGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassArray", 1, ezGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("ClassPtrArray", 0);
    B = CreateGuid("ClassPtrArray", 1);
    C = CreateGuid("ClassPtrArray", 2);

    TestAddObject(pObjectTest, "ClassPtrArray", 0, ezGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassPtrArray", 1, ezGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassPtrArray", 1, ezGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("SubObjectSet", 0);
    B = CreateGuid("SubObjectSet", 1);
    C = CreateGuid("SubObjectSet", 2);

    TestAddObject(pObjectTest, "SubObjectSet", 0, ezGetStaticRTTI<ezObjectTest>(), A);
    TestAddObject(pObjectTest, "SubObjectSet", 1, ezGetStaticRTTI<ezObjectTest>(), C);
    TestAddObject(pObjectTest, "SubObjectSet", 1, ezGetStaticRTTI<ezObjectTest>(), B);

    A = CreateGuid("ClassMap", 0);
    B = CreateGuid("ClassMap", 1);
    C = CreateGuid("ClassMap", 2);

    TestAddObject(pObjectTest, "ClassMap", "A", ezGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassMap", "C", ezGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassMap", "B", ezGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("ClassPtrMap", 0);
    B = CreateGuid("ClassPtrMap", 1);
    C = CreateGuid("ClassPtrMap", 2);

    TestAddObject(pObjectTest, "ClassPtrMap", "A", ezGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassPtrMap", "C", ezGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassPtrMap", "B", ezGetStaticRTTI<OuterClass>(), B);
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MoveObject")
  {
    auto TestMoveObjectFailure = [&](const ezDocumentObject* pObject, const char* szProperty, ezVariant newIndex)
    {
      pAccessor->StartTransaction("MoveObject");
      EZ_TEST_BOOL(pAccessor->MoveObjectByName(pObject, pObject->GetParent(), szProperty, newIndex).Failed());
      pAccessor->CancelTransaction();
    };

    auto TestMoveObject = [&](const ezDocumentObject* pObject, const char* szProperty, ezVariant newIndex, ezArrayPtr<ezUuid> expectedOutcome)
    {
      ezAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject->GetParent());

      const ezUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const ezInt32 iArraySize = pAccessor->GetCountByName(pObject->GetParent(), szProperty);
      EZ_TEST_INT(iArraySize, expectedOutcome.GetCount());

      ezDynamicArray<ezVariant> values;
      EZ_TEST_STATUS(pAccessor->GetValuesByName(pObject->GetParent(), szProperty, values));
      EZ_TEST_INT(iArraySize, values.GetCount());

      pAccessor->StartTransaction("MoveObject");
      EZ_TEST_STATUS(pAccessor->MoveObjectByName(pObject, pObject->GetParent(), szProperty, newIndex));
      pAccessor->FinishTransaction();
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject->GetParent(), szProperty), iArraySize);

      for (ezInt32 i = 0; i < iArraySize; i++)
      {
        ezVariant newValue;
        EZ_TEST_STATUS(pAccessor->GetValueByName(pObject->GetParent(), szProperty, newValue, i));
        EZ_TEST_BOOL(newValue == expectedOutcome[i]);
      }

      EZ_TEST_STATUS(doc.GetCommandHistory()->Undo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject->GetParent(), szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject->GetParent());

      EZ_TEST_STATUS(doc.GetCommandHistory()->Redo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pObject->GetParent(), szProperty), iArraySize);

      for (ezInt32 i = 0; i < iArraySize; i++)
      {
        ezVariant newValue;
        EZ_TEST_STATUS(pAccessor->GetValueByName(pObject->GetParent(), szProperty, newValue, i));
        EZ_TEST_BOOL(newValue == expectedOutcome[i]);
      }
    };

    ezUuid A = CreateGuid("ClassArray", 0);
    ezUuid B = CreateGuid("ClassArray", 1);
    ezUuid C = CreateGuid("ClassArray", 2);
    const ezDocumentObject* pA = pAccessor->GetObject(A);
    const ezDocumentObject* pB = pAccessor->GetObject(B);
    const ezDocumentObject* pC = pAccessor->GetObject(C);

    {
      // Move first element before or after itself (no-op)
      TestMoveObjectFailure(pA, "ClassArray", 0);
      TestMoveObjectFailure(pA, "ClassArray", 1);
      // Move last element before or after itself (no-op)
      TestMoveObjectFailure(pC, "ClassArray", 2);
      TestMoveObjectFailure(pC, "ClassArray", 3);
    }
    {
      // Move first element to the end.
      ezUuid expectedValues[3] = {B, C, A};
      TestMoveObject(pA, "ClassArray", 3, ezArrayPtr<ezUuid>(expectedValues));
    }
    {
      // Move last element to the front.
      ezUuid expectedValues[3] = {A, B, C};
      TestMoveObject(pA, "ClassArray", 0, ezArrayPtr<ezUuid>(expectedValues));
    }
    {
      // Move first element to the middle
      ezUuid expectedValues[3] = {B, A, C};
      TestMoveObject(pA, "ClassArray", 2, ezArrayPtr<ezUuid>(expectedValues));
    }
    {
      // Move last element to the middle
      ezUuid expectedValues[3] = {B, C, A};
      TestMoveObject(pC, "ClassArray", 1, ezArrayPtr<ezUuid>(expectedValues));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveObject")
  {
    auto TestRemoveObject = [&](const ezDocumentObject* pObject, ezArrayPtr<ezUuid> expectedOutcome)
    {
      auto pParent = pObject->GetParent();
      ezString sProperty = pObject->GetParentProperty();

      ezAbstractObjectGraph graph;
      StoreOriginalState(graph, pParent);
      const ezUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const ezInt32 iArraySize = pAccessor->GetCountByName(pParent, sProperty);
      EZ_TEST_INT(iArraySize - 1, expectedOutcome.GetCount());


      ezDynamicArray<ezVariant> values;
      ezDynamicArray<ezVariant> keys;
      {
        EZ_TEST_STATUS(pAccessor->GetValuesByName(pParent, sProperty, values));
        EZ_TEST_INT(iArraySize, values.GetCount());

        EZ_TEST_STATUS(pAccessor->GetKeysByName(pParent, sProperty, keys));
        EZ_TEST_INT(iArraySize, keys.GetCount());
        ezUInt32 uiIndex = keys.IndexOf(pObject->GetPropertyIndex());
        keys.RemoveAtAndSwap(uiIndex);
        values.RemoveAtAndSwap(uiIndex);
        EZ_TEST_INT(iArraySize - 1, keys.GetCount());
      }

      pAccessor->StartTransaction("RemoveValue");
      EZ_TEST_STATUS(pAccessor->RemoveObject(pObject));
      pAccessor->FinishTransaction();
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pParent, sProperty), iArraySize - 1);

      if (pParent->GetType()->FindPropertyByName(sProperty)->GetCategory() == ezPropertyCategory::Map)
      {
        for (ezInt32 i = 0; i < iArraySize - 1; i++)
        {
          const ezVariant& key = keys[i];
          const ezVariant& value = values[i];
          ezVariant newValue;
          EZ_TEST_STATUS(pAccessor->GetValueByName(pParent, sProperty, newValue, key));
          EZ_TEST_BOOL(newValue == value);
          EZ_TEST_BOOL(expectedOutcome.Contains(newValue.Get<ezUuid>()));
        }
      }
      else
      {
        for (ezInt32 i = 0; i < iArraySize - 1; i++)
        {
          ezVariant newValue;
          EZ_TEST_STATUS(pAccessor->GetValueByName(pParent, sProperty, newValue, i));
          EZ_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }

      EZ_TEST_STATUS(doc.GetCommandHistory()->Undo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      EZ_TEST_INT(pAccessor->GetCountByName(pParent, sProperty), iArraySize);
      CompareAgainstOriginalState(graph, pParent);

      EZ_TEST_STATUS(doc.GetCommandHistory()->Redo());
      EZ_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      EZ_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      EZ_TEST_INT(pAccessor->GetCountByName(pParent, sProperty), iArraySize - 1);

      if (pParent->GetType()->FindPropertyByName(sProperty)->GetCategory() == ezPropertyCategory::Map)
      {
        for (ezInt32 i = 0; i < iArraySize - 1; i++)
        {
          const ezVariant& key = keys[i];
          const ezVariant& value = values[i];
          ezVariant newValue;
          EZ_TEST_STATUS(pAccessor->GetValueByName(pParent, sProperty, newValue, key));
          EZ_TEST_BOOL(newValue == value);
          EZ_TEST_BOOL(expectedOutcome.Contains(newValue.Get<ezUuid>()));
        }
      }
      else
      {
        for (ezInt32 i = 0; i < iArraySize - 1; i++)
        {
          ezVariant newValue;
          EZ_TEST_STATUS(pAccessor->GetValueByName(pParent, sProperty, newValue, i));
          EZ_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }
    };

    auto ClearContainer = [&](const char* szContainer)
    {
      ezUuid A = CreateGuid(szContainer, 0);
      ezUuid B = CreateGuid(szContainer, 1);
      ezUuid C = CreateGuid(szContainer, 2);
      const ezDocumentObject* pA = pAccessor->GetObject(A);
      const ezDocumentObject* pB = pAccessor->GetObject(B);
      const ezDocumentObject* pC = pAccessor->GetObject(C);
      {
        ezUuid expectedValues[2] = {B, C};
        TestRemoveObject(pA, ezArrayPtr<ezUuid>(expectedValues));
      }
      {
        ezUuid expectedValues[1] = {C};
        TestRemoveObject(pB, ezArrayPtr<ezUuid>(expectedValues));
      }
      {
        TestRemoveObject(pC, ezArrayPtr<ezUuid>());
      }
    };

    ClearContainer("ClassArray");
    ClearContainer("ClassPtrArray");
    ClearContainer("SubObjectSet");
    ClearContainer("ClassMap");
    ClearContainer("ClassPtrMap");
  }
}
