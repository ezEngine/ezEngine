#include <PCH.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void MirrorCheck(ezTestDocument* pDoc, const ezDocumentObject* pObject)
{
  // Create native object graph
  ezAbstractObjectGraph graph;
  ezAbstractObjectNode* pRootNode = nullptr;
  {
    ezRttiConverterWriter rttiConverter(&graph, &pDoc->m_Context, true, true);
    pRootNode = rttiConverter.AddObjectToGraph(pObject->GetType(), pDoc->m_ObjectMirror.GetNativeObjectPointer(pObject), "Object");
  }

  // Create object manager graph
  ezAbstractObjectGraph origGraph;
  ezAbstractObjectNode* pOrigRootNode = nullptr;
  {
    ezDocumentObjectConverterWriter writer(&origGraph, pDoc->GetObjectManager(), true, true);
    pOrigRootNode = writer.AddObjectToGraph(pObject);
  }

  // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
  graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  ezDeque<ezAbstractGraphDiffOperation> diffResult;

  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  EZ_TEST_BOOL(diffResult.GetCount() == 0);
}


ezVariant GetVariantFromType(ezVariant::Type::Enum type)
{
  switch (type)
  {
  case ezVariant::Type::Invalid:
    return ezVariant();
  case ezVariant::Type::Bool:
    return ezVariant(true);
  case ezVariant::Type::Int8:
    return ezVariant((ezInt8)-55);
  case ezVariant::Type::UInt8:
    return ezVariant((ezUInt8)44);
  case ezVariant::Type::Int16:
    return ezVariant((ezInt16)-444);
  case ezVariant::Type::UInt16:
    return ezVariant((ezUInt16)666);
  case ezVariant::Type::Int32:
    return ezVariant((ezInt32)-88880);
  case ezVariant::Type::UInt32:
    return ezVariant((ezUInt32)123445);
  case ezVariant::Type::Int64:
    return ezVariant((ezInt64)-888800000);
  case ezVariant::Type::UInt64:
    return ezVariant((ezUInt64)123445000);
  case ezVariant::Type::Float:
    return ezVariant(1024.0f);
  case ezVariant::Type::Double:
    return ezVariant(-2048.0f);
  case ezVariant::Type::Color:
    return ezVariant(ezColor(0.5f, 33.0f, 2.0f, 0.3f));
  case ezVariant::Type::Vector2:
    return ezVariant(ezVec2(2.0f, 4.0f));
  case ezVariant::Type::Vector3:
    return ezVariant(ezVec3(2.0f, 4.0f, -8.0f));
  case ezVariant::Type::Vector4:
    return ezVariant(ezVec4(1.0f, 7.0f, 8.0f, -10.0f));
  case ezVariant::Type::Vector2I:
    return ezVariant(ezVec2I32(1, 2));
  case ezVariant::Type::Vector3I:
    return ezVariant(ezVec3I32(3, 4, 5));
  case ezVariant::Type::Vector4I:
    return ezVariant(ezVec4I32(6, 7, 8, 9));
  case ezVariant::Type::Quaternion:
    {
      ezQuat quat;
      quat.SetFromEulerAngles(ezAngle::Degree(30), ezAngle::Degree(-15), ezAngle::Degree(20));
      return ezVariant(quat);
    }
  case ezVariant::Type::Matrix3:
    {
      ezMat3 mat = ezMat3::IdentityMatrix();
      
      mat.SetRotationMatrix(ezVec3(1.0f, 0.0f, 0.0f), ezAngle::Degree(30));
      return ezVariant(mat);
    }
  case ezVariant::Type::Matrix4:
    {
      ezMat4 mat = ezMat4::IdentityMatrix();

      mat.SetRotationMatrix(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(30));
      mat.SetTranslationVector(ezVec3(1.0f, 2.0f, 3.0f));
      return ezVariant(mat);
    }
  case ezVariant::Type::String:
    return ezVariant("Test");
  case ezVariant::Type::StringView:
    return ezVariant("Test");
  case ezVariant::Type::Time:
    return ezVariant(ezTime::Seconds(123.0f));
  case ezVariant::Type::Uuid:
    {
      ezUuid guid;
      guid.CreateNewUuid();
      return ezVariant(guid);
    }
  case ezVariant::Type::Angle:
    return ezVariant(ezAngle::Degree(30.0f));
  case ezVariant::Type::DataBuffer:
    {
      ezDataBuffer data;
      data.PushBack(12);
      data.PushBack(55);
      data.PushBack(88);
      return ezVariant(data);
    }
  case ezVariant::Type::VariantArray:
    return ezVariantArray();
  case ezVariant::Type::VariantDictionary:
    return ezVariantDictionary();
  case ezVariant::Type::ReflectedPointer:
    return ezVariant();
  case ezVariant::Type::VoidPointer:
    return ezVariant();
  case ezVariant::Type::ENUM_COUNT:
    EZ_REPORT_FAILURE("Invalid case statement");
    return ezVariant();
  }
  return ezVariant();
}

void RecursiveModifyProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezObjectAccessorBase* pObjectAccessor)
{
  if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      const ezUuid oldGuid = pObjectAccessor->Get<ezUuid>(pObject, pProp);
      ezUuid newGuid;
      newGuid.CreateNewUuid();
      if (oldGuid.IsValid())
      {
        EZ_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(oldGuid)).m_Result.Succeeded());
      }

      EZ_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, ezVariant(), pProp->GetSpecificType(), newGuid).m_Result.Succeeded());

      const ezDocumentObject* pChild = pObject->GetChild(newGuid);
      EZ_ASSERT_DEV(pChild != nullptr, "References child object does not exist!");
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags | ezPropertyFlags::StandardType | ezPropertyFlags::Pointer))
    {
      ezVariant value = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
      EZ_TEST_BOOL(pObjectAccessor->SetValue(pObject, pProp, value).m_Result.Succeeded());
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::EmbeddedClass))
    {
      // Noting to do here, value cannot change
    }
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    ezInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      for (ezInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        pObjectAccessor->RemoveValue(pObject, pProp, i);
      }

      ezVariant value1 = ezToolsReflectionUtils::GetDefaultValue(pProp);
      ezVariant value2 = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
      EZ_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value1, 0).m_Result.Succeeded());
      EZ_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value2, 1).m_Result.Succeeded());
    }
    else
    {
      ezInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      ezHybridArray<ezVariant, 16> currentValues;
      pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
      for (ezInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        EZ_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<ezUuid>())).m_Result.Succeeded());
      }

      if (pProp->GetCategory() == ezPropertyCategory::Array && pProp->GetFlags().IsAnySet(ezPropertyFlags::EmbeddedClass))
      {
        ezUuid newGuid;
        newGuid.CreateNewUuid();
        EZ_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, 0, pProp->GetSpecificType(), newGuid).m_Result.Succeeded());
      }
      //pObjectAccessor->AddObject(pObject, pProp, 0, ezRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node);
    }
  }
}

void RecursiveModifyObject(const ezDocumentObject* pObject, ezObjectAccessorBase* pAccessor)
{
  ezHybridArray<ezAbstractProperty*, 32> Properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(Properties);
  for (const auto* pProp : Properties)
  {
    RecursiveModifyProperty(pObject, pProp, pAccessor);
  }

  for (const ezDocumentObject* pSubObject : pObject->GetChildren())
  {
    RecursiveModifyObject(pSubObject, pAccessor);
  }
}


EZ_CREATE_SIMPLE_TEST_GROUP(ObjectMirror);

EZ_CREATE_SIMPLE_TEST(ObjectMirror, ObjectMirror)
{
  ezTestDocument doc("Test", true);
  doc.InitializeAfterLoading();
  ezTestDocumentObjectManager* pObjectManager = static_cast<ezTestDocumentObjectManager*>(doc.GetObjectManager());
  ezObjectAccessorBase* pAccessor = doc.GetObjectAccessor();
  ezUuid mirrorGuid;

  pAccessor->StartTransaction("Init");
  ezStatus status = pAccessor->AddObject(nullptr, nullptr, -1, ezGetStaticRTTI<ezMirrorTest>(), mirrorGuid);
  const ezDocumentObject* pObject = pAccessor->GetObject(mirrorGuid);
  EZ_TEST_BOOL(status.m_Result.Succeeded());
  pAccessor->FinishTransaction();

  MirrorCheck(&doc, pObject);
  ezMirrorTest* pNative = static_cast<ezMirrorTest*>(doc.m_ObjectMirror.GetNativeObjectPointer(pObject));

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Document Changes")
  {
    pAccessor->StartTransaction("Document Changes");
    RecursiveModifyObject(pObject, pAccessor);
    pAccessor->FinishTransaction();

    MirrorCheck(&doc, pObject);
  }
  {
    pAccessor->StartTransaction("Document Changes");
    RecursiveModifyObject(pObject, pAccessor);
    pAccessor->FinishTransaction();

    MirrorCheck(&doc, pObject);
  }
}

