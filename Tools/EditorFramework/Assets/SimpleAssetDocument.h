#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

template<typename PropertyType, typename AssetObjectType, typename ObjectManagerType>
class ezSimpleAssetDocument : public ezAssetDocument
{
public:
  ezSimpleAssetDocument(const char* szDocumentPath) : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ObjectManagerType))
  {

  }

  const PropertyType* GetProperties() const
  {
    AssetObjectType* pObject = static_cast<AssetObjectType*>(GetObjectManager()->GetRootObject()->GetChildren()[0]);
    return &pObject->m_MemberProperties;
  }

protected:
  virtual void InitializeAfterLoading() override
  {
    ezAssetDocument::InitializeAfterLoading();

    EnsureSettingsObjectExist();
  }

  virtual ezStatus InternalLoadDocument() override
  {
    GetObjectManager()->DestroyAllObjects(GetObjectManager());

    ezStatus ret = ezAssetDocument::InternalLoadDocument();

    EnsureSettingsObjectExist();

    return ret;
  }

private:
  void EnsureSettingsObjectExist()
  {
    auto pRoot = GetObjectManager()->GetRootObject();
    if (pRoot->GetChildren().IsEmpty())
    {
      AssetObjectType* pObject = static_cast<AssetObjectType*>(GetObjectManager()->CreateObject(ezRTTI::FindTypeByName(ezGetStaticRTTI<PropertyType>()->GetTypeName())));
      GetObjectManager()->AddObject(pObject, pRoot);
    }
  }

  virtual ezDocumentInfo* CreateDocumentInfo() override 
  { 
    return EZ_DEFAULT_NEW(ezAssetDocumentInfo); 
  }
};




template<typename ObjectProperties, typename ObjectType>
class ezSimpleDocumentObjectManager : public ezDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(ezHybridArray<ezRTTI*, 32>& Types) const override
  {
    Types.PushBack(ezRTTI::FindTypeByName(ezGetStaticRTTI<ObjectProperties>()->GetTypeName()));
  }

private:

  virtual ezDocumentObjectBase* InternalCreateObject(const ezRTTI* pRtti) override
  {
    return EZ_DEFAULT_NEW(ObjectType);
  }

  virtual void InternalDestroyObject(ezDocumentObjectBase* pObject) override
  {
    EZ_DEFAULT_DELETE(pObject);
  }

  virtual bool InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent) const override { return true; }
  virtual bool InternalCanRemove(const ezDocumentObjectBase* pObject) const override { return true; }
  virtual bool InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const override { return false; }
};