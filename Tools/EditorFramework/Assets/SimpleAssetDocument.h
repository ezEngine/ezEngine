#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

template<typename PropertyType>
class ezSimpleAssetDocument : public ezAssetDocument
{
public:
  ezSimpleAssetDocument(const char* szDocumentPath, bool bUseEngineConnection = false, bool bUseIPCObjectMirror = false)
    : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezSimpleDocumentObjectManager<PropertyType>), bUseEngineConnection, bUseIPCObjectMirror)
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
    ezAssetDocument::InitializeAfterLoading();

    EnsureSettingsObjectExist();

    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();
  }

  virtual ezStatus InternalLoadDocument() override
  {
    GetObjectManager()->DestroyAllObjects();

    ezStatus ret = ezAssetDocument::InternalLoadDocument();

    return ret;
  }

  void ApplyNativePropertyChangesToObjectManager()
  {
    // Create native object graph
    ezAbstractObjectGraph graph;
    ezAbstractObjectNode* pRootNode = nullptr;
    {
      ezRttiConverterWriter rttiConverter(&graph, &m_Context, true, true);
      pRootNode = rttiConverter.AddObjectToGraph(GetProperties(), "Object");
    }

    // Create object manager graph
    ezAbstractObjectGraph origGraph;
    ezAbstractObjectNode* pOrigRootNode = nullptr;
    {
      ezDocumentObjectConverterWriter writer(&origGraph, GetObjectManager(), true, true);
      pOrigRootNode = writer.AddObjectToGraph(GetPropertyObject());
    }

    // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
    graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
    ezDeque<ezAbstractGraphDiffOperation> diffResult;

    graph.CreateDiffWithBaseGraph(origGraph, diffResult);

    // As we messed up the native side the object mirror is no longer synced and needs to be destroyed.
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();

    // Apply diff while object mirror is down.
    GetObjectAccessor()->StartTransaction("Apply Native Property Changes to Object");
    ezDocumentObjectConverterReader::ApplyDiffToObject(GetObjectAccessor(), GetPropertyObject(), diffResult);
    GetObjectAccessor()->FinishTransaction();

    // Restart mirror from scratch.
    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
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

private:
  ezDocumentObjectMirror m_ObjectMirror;
  ezRttiConverterContext m_Context;
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