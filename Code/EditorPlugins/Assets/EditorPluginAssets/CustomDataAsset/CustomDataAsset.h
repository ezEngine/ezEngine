#pragma once

#include <Core/Utils/CustomData.h>
#include <EditorFramework/Assets/AssetDocument.h>


class ezCustomDataDocumentObjectManager : public ezDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const override;
};


class ezCustomDataAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomDataAssetDocument, ezAssetDocument);

public:
  ezCustomDataAssetDocument(const ezRTTI* pDataType, ezStringView sDocumentPath);
  ~ezCustomDataAssetDocument();

  const ezCustomData* GetProperties() const
  {
    return static_cast<const ezCustomData*>(m_ObjectMirror.GetNativeObjectPointer(this->GetObjectManager()->GetRootObject()->GetChildren()[0]));
  }

  ezCustomData* GetProperties()
  {
    return static_cast<ezCustomData*>(m_ObjectMirror.GetNativeObjectPointer(this->GetObjectManager()->GetRootObject()->GetChildren()[0]));
  }

  ezDocumentObject* GetPropertyObject() { return this->GetObjectManager()->GetRootObject()->GetChildren()[0]; }

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual ezStatus InternalLoadDocument() override;

  void ApplyNativePropertyChangesToObjectManager(bool bForceIndexBasedRemapping = false);

private:
  void EnsureSettingsObjectExist();

protected:
  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezAssetDocumentInfo); }

  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
                                                   const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;


  ezDocumentObjectMirror m_ObjectMirror;
  ezRttiConverterContext m_Context;
  ezEngineViewLightSettings m_LightSettings;
  const ezRTTI* m_pDataType;
};

