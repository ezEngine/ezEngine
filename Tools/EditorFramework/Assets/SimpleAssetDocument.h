#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class ezApplyNativePropertyChangesContext : public ezRttiConverterContext
{
public:
  ezApplyNativePropertyChangesContext(ezRttiConverterContext& source, const ezAbstractObjectGraph& originalGraph)
    : m_nativeContext(source)
    , m_originalGraph(originalGraph)
  {
  }

  virtual ezUuid GenerateObjectGuid(const ezUuid& parentGuid, const ezAbstractProperty* pProp, ezVariant index, void* pObject) const override
  {
    ezUuid guid;
    if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      // If the object is already known by the native context (a pointer that existed before the native changes)
      // we can just return it. Any other pointer will get a new guid assigned.
      guid = m_nativeContext.GetObjectGUID(pProp->GetSpecificType(), pObject);
      if (guid.IsValid())
        return guid;
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
    {
      // In case of by-value classes we lookup the guid in the object manager graph by using
      // the index as the identify of the object. If the index is not valid (e.g. the array was expanded by native changes)
      // a new guid is assigned.
      if (const ezAbstractObjectNode* originalNode = m_originalGraph.GetNode(parentGuid))
      {
        if (const ezAbstractObjectNode::Property* originalProp = originalNode->FindProperty(pProp->GetPropertyName()))
        {
          switch (pProp->GetCategory())
          {
          case ezPropertyCategory::Member:
            {
              if (originalProp->m_Value.IsA<ezUuid>() && originalProp->m_Value.Get<ezUuid>().IsValid())
                return originalProp->m_Value.Get<ezUuid>();
            }
            break;
          case ezPropertyCategory::Array:
            {
              ezUInt32 uiIndex = index.Get<ezUInt32>();
              if (originalProp->m_Value.IsA<ezVariantArray>())
              {
                const ezVariantArray& values = originalProp->m_Value.Get<ezVariantArray>();
                if (uiIndex < values.GetCount())
                {
                  const auto& originalElemValue = values[uiIndex];
                  if (originalElemValue.IsA<ezUuid>() && originalElemValue.Get<ezUuid>().IsValid())
                    return originalElemValue.Get<ezUuid>();
                }
              }
            }
            break;
          case ezPropertyCategory::Map:
            {
              const ezString& sIndex = index.Get<ezString>();
              if (originalProp->m_Value.IsA<ezVariantDictionary>())
              {
                const ezVariantDictionary& values = originalProp->m_Value.Get<ezVariantDictionary>();
                if (values.Contains(sIndex))
                {
                  const auto& originalElemValue = *values.GetValue(sIndex);
                  if (originalElemValue.IsA<ezUuid>() && originalElemValue.Get<ezUuid>().IsValid())
                    return originalElemValue.Get<ezUuid>();
                }
              }
            }
            break;
          }
        }
      }

    }
    guid.CreateNewUuid();
    return guid;
  }
private:
  ezRttiConverterContext& m_nativeContext;
  const ezAbstractObjectGraph& m_originalGraph;
};

template<typename ObjectProperties>
class ezSimpleDocumentObjectManager : public ezDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override
  {
    Types.PushBack(ezGetStaticRTTI<ObjectProperties>());
  }
};

template<typename PropertyType, typename BaseClass = ezAssetDocument>
class ezSimpleAssetDocument : public BaseClass
{
public:
  ezSimpleAssetDocument(const char* szDocumentPath, bool bUseEngineConnection = false, bool bUseIPCObjectMirror = false)
    : BaseClass(szDocumentPath, EZ_DEFAULT_NEW(ezSimpleDocumentObjectManager<PropertyType>), bUseEngineConnection, bUseIPCObjectMirror)
  {
  }

  ezSimpleAssetDocument(ezDocumentObjectManager* pObjectManager, const char* szDocumentPath, bool bUseEngineConnection = false, bool bUseIPCObjectMirror = false)
    : BaseClass(szDocumentPath, pObjectManager, bUseEngineConnection, bUseIPCObjectMirror)
  {
  }

  ~ezSimpleAssetDocument()
  {
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();
  }

  const PropertyType* GetProperties() const
  {
    return static_cast<const PropertyType*>(m_ObjectMirror.GetNativeObjectPointer(GetObjectManager()->GetRootObject()->GetChildren()[0]));
  }

  PropertyType* GetProperties()
  {
    return static_cast<PropertyType*>(m_ObjectMirror.GetNativeObjectPointer(GetObjectManager()->GetRootObject()->GetChildren()[0]));
  }

  ezDocumentObject* GetPropertyObject()
  {
    return GetObjectManager()->GetRootObject()->GetChildren()[0];
  }

protected:
  virtual void InitializeAfterLoading() override
  {
    EnsureSettingsObjectExist();

    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();

    BaseClass::InitializeAfterLoading();
  }

  virtual ezStatus InternalLoadDocument() override
  {
    GetObjectManager()->DestroyAllObjects();

    ezStatus ret = BaseClass::InternalLoadDocument();

    return ret;
  }

  // Index based remapping ignores address identity and solely uses the object's parent index to define
  // its guid. Set it to true if the native changes are complete clear and replace operations and
  // not incremental changes to the existing data.
  void ApplyNativePropertyChangesToObjectManager(bool bForceIndexBasedRemapping = false)
  {
    // Create object manager graph
    ezAbstractObjectGraph origGraph;
    ezAbstractObjectNode* pOrigRootNode = nullptr;
    {
      ezDocumentObjectConverterWriter writer(&origGraph, GetObjectManager());
      pOrigRootNode = writer.AddObjectToGraph(GetPropertyObject());
    }

    // Create native object graph
    ezAbstractObjectGraph graph;
    ezAbstractObjectNode* pRootNode = nullptr;
    {
      // The ezApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those
      // of the object manager.
      ezApplyNativePropertyChangesContext nativeChangesContext(m_Context, origGraph);
      ezRttiConverterWriter rttiConverter(&graph, &nativeChangesContext, true, true);
      nativeChangesContext.RegisterObject(pOrigRootNode->GetGuid(), ezGetStaticRTTI<PropertyType>(), GetProperties());
      pRootNode = rttiConverter.AddObjectToGraph(GetProperties(), "Object");
    }

    // Remapping is no longer necessary as ezApplyNativePropertyChangesContext takes care of mapping to the original nodes.
    // However, if the native changes are done like clear+rebuild everything, then no original object will be found and
    // every pointer will be deleted and re-created. Forcing the remapping (which works entirely via index and ignores
    // pointer addresses) will yield better results (e.g. no changes on two back-to -back transform calls).
    if (bForceIndexBasedRemapping)
    {
      // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
      graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
    }

    ezDeque<ezAbstractGraphDiffOperation> diffResult;
    graph.CreateDiffWithBaseGraph(origGraph, diffResult);

    // As we messed up the native side the object mirror is no longer synced and needs to be destroyed.
    m_ObjectMirror.Clear();
    // Apply diff while object mirror is down.
    GetObjectAccessor()->StartTransaction("Apply Native Property Changes to Object");
    ezDocumentObjectConverterReader::ApplyDiffToObject(GetObjectAccessor(), GetPropertyObject(), diffResult);
    GetObjectAccessor()->FinishTransaction();
    // Re-apply document
    m_ObjectMirror.SendDocument();
  }

private:
  void EnsureSettingsObjectExist()
  {
    auto pRoot = GetObjectManager()->GetRootObject();
    if (pRoot->GetChildren().IsEmpty())
    {
      ezDocumentObject* pObject = GetObjectManager()->CreateObject(ezGetStaticRTTI<PropertyType>());
      GetObjectManager()->AddObject(pObject, pRoot, "Children", 0);
    }
  }

  virtual ezDocumentInfo* CreateDocumentInfo() override
  {
    return EZ_DEFAULT_NEW(ezAssetDocumentInfo);
  }

protected:
  ezDocumentObjectMirror m_ObjectMirror;
  ezRttiConverterContext m_Context;
};
