#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlackboardTemplateAssetObject, 1, ezRTTIDefaultAllocator<ezBlackboardTemplateAssetObject>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("BaseTemplates", m_BaseTemplates)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate", ezDependencyFlags::Transform)),
    EZ_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlackboardTemplateAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBlackboardTemplateAssetDocument::ezBlackboardTemplateAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezBlackboardTemplateAssetObject>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezStatus ezBlackboardTemplateAssetDocument::WriteAsset(ezStreamWriter& inout_stream, const ezPlatformProfile* pAssetProfile) const
{
  ezBlackboardTemplateResourceDescriptor desc;
  EZ_SUCCEED_OR_RETURN(RetrieveState(GetProperties(), desc));
  EZ_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezBlackboardTemplateAssetDocument::RetrieveState(const ezBlackboardTemplateAssetObject* pProp, ezBlackboardTemplateResourceDescriptor& inout_Desc) const
{
  for (const ezString& sTempl : pProp->m_BaseTemplates)
  {
    if (sTempl.IsEmpty())
      continue;

    auto pOther = ezAssetCurator::GetSingleton()->FindSubAsset(sTempl);
    if (!pOther.isValid())
    {
      return ezStatus(ezFmt("Base template '{}' not found.", sTempl));
    }

    ezDocument* pDoc;
    EZ_SUCCEED_OR_RETURN(pOther->m_pAssetInfo->GetManager()->OpenDocument(pOther->m_Data.m_sSubAssetsDocumentTypeName, pOther->m_pAssetInfo->m_Path, pDoc, ezDocumentFlags::None, nullptr));

    if (ezBlackboardTemplateAssetDocument* pTmpDoc = ezDynamicCast<ezBlackboardTemplateAssetDocument*>(pDoc))
    {
      EZ_SUCCEED_OR_RETURN(RetrieveState(pTmpDoc->GetProperties(), inout_Desc));
    }

    pOther->m_pAssetInfo->GetManager()->CloseDocument(pDoc);
  }

  for (const auto& e : pProp->m_Entries)
  {
    for (auto& e2 : inout_Desc.m_Entries)
    {
      if (e2.m_sName == e.m_sName)
      {
        e2 = e;
        goto next;
      }
    }

    inout_Desc.m_Entries.PushBack(e);

  next:;
  }

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezBlackboardTemplateAssetDocument::InternalTransformAsset(ezStreamWriter& inout_stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  return WriteAsset(inout_stream, pAssetProfile);
}
