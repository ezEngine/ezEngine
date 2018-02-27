#include <PCH.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <EnginePluginScene/RenderPipeline/EditorShapeIconsExtractor.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorShapeIconsExtractor, 1, ezRTTIDefaultAllocator<ezEditorShapeIconsExtractor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f), new ezSuffixAttribute(" m")),
    EZ_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(64.0f), new ezSuffixAttribute(" px")),
    EZ_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


ezEditorShapeIconsExtractor::ezEditorShapeIconsExtractor(const char* szName)
  : ezExtractor(szName)
{
  m_fSize = 1.0f;
  m_fMaxScreenSize = 64.0f;
  m_pSceneContext = nullptr;

  FillShapeIconInfo();
}

ezEditorShapeIconsExtractor::~ezEditorShapeIconsExtractor()
{
}

void ezEditorShapeIconsExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
  ezExtractedRenderData* pExtractedRenderData)
{
  EZ_LOCK(view.GetWorld()->GetReadMarker());

  /// \todo Once we have a solution for objects that only have a shape icon we can switch this loop to use visibleObjects instead.
  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
  {
    const ezGameObject* pObject = it;
    if (FilterByViewTags(view, pObject))
      continue;

    ExtractShapeIcon(pObject, view, pExtractedRenderData, ezDefaultRenderDataCategories::SimpleOpaque);
  }

  if (m_pSceneContext != nullptr)
  {
    auto objects = m_pSceneContext->GetSelectionWithChildren();

    for (const auto& hObject : objects)
    {
      const ezGameObject* pObject = nullptr;
      if (view.GetWorld()->TryGetObject(hObject, pObject))
      {
        if (FilterByViewTags(view, pObject))
          continue;

        ExtractShapeIcon(pObject, view, pExtractedRenderData, ezDefaultRenderDataCategories::Selection);
      }
    }
  }
}

void ezEditorShapeIconsExtractor::ExtractShapeIcon(const ezGameObject* pObject, const ezView& view, ezExtractedRenderData* pExtractedRenderData, ezRenderData::Category category)
{
  static const ezTag& tagHidden = ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden");
  static const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  static const ezTag& tagPrefab = ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorPrefabInstance");

  if (pObject->GetTags().IsSet(tagEditor) || pObject->GetTags().IsSet(tagHidden) || pObject->GetTags().IsSet(tagPrefab))
    return;

  if (pObject->GetComponents().IsEmpty())
    return;

  const ezComponent* pComponent = pObject->GetComponents()[0];
  const ezRTTI* pRtti = pComponent->GetDynamicRTTI();

  ShapeIconInfo* pShapeIconInfo = nullptr;
  if (m_ShapeIconInfos.TryGetValue(pRtti, pShapeIconInfo))
  {
    const ezUInt32 uiTextureIDHash = pShapeIconInfo->m_hTexture.GetResourceIDHash();
    ezSpriteRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezSpriteRenderData>(pObject, uiTextureIDHash);
    {
      pRenderData->m_GlobalTransform = pObject->GetGlobalTransform();
      pRenderData->m_GlobalBounds = pObject->GetGlobalBounds();
      pRenderData->m_hTexture = pShapeIconInfo->m_hTexture;
      pRenderData->m_fSize = m_fSize;
      pRenderData->m_fMaxScreenSize = m_fMaxScreenSize;
      pRenderData->m_texCoordScale = ezVec2(1.0f);
      pRenderData->m_texCoordOffset = ezVec2(0.0f);
      pRenderData->m_uiUniqueID = ezRenderComponent::GetUniqueIdForRendering(pComponent);

      // prefer color gamma properties
      if (pShapeIconInfo->m_pColorGammaProperty != nullptr)
      {
        pRenderData->m_color = ezColor(pShapeIconInfo->m_pColorGammaProperty->GetValue(pComponent));
      }
      else if (pShapeIconInfo->m_pColorProperty != nullptr)
      {
        pRenderData->m_color = pShapeIconInfo->m_pColorProperty->GetValue(pComponent);
      }
      else
      {
        pRenderData->m_color = ezColor::White;
      }

      pRenderData->m_color.a = 255;
    }

    pExtractedRenderData->AddRenderData(pRenderData, category, uiTextureIDHash);
  }
}

const ezTypedMemberProperty<ezColor>* ezEditorShapeIconsExtractor::FindColorProperty(const ezRTTI* pRtti) const
{
  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const ezAbstractProperty* pProperty : properties)
  {
    if (pProperty->GetCategory() == ezPropertyCategory::Member &&
      pProperty->GetSpecificType() == ezGetStaticRTTI<ezColor>())
    {
      return static_cast<const ezTypedMemberProperty<ezColor>*>(pProperty);
    }
  }

  return nullptr;
}

const ezTypedMemberProperty<ezColorGammaUB>* ezEditorShapeIconsExtractor::FindColorGammaProperty(const ezRTTI* pRtti) const
{
  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const ezAbstractProperty* pProperty : properties)
  {
    if (pProperty->GetCategory() == ezPropertyCategory::Member &&
        pProperty->GetSpecificType() == ezGetStaticRTTI<ezColorGammaUB>())
    {
      return static_cast<const ezTypedMemberProperty<ezColorGammaUB>*>(pProperty);
    }
  }

  return nullptr;
}

void ezEditorShapeIconsExtractor::FillShapeIconInfo()
{
  EZ_LOG_BLOCK("LoadShapeIconTextures");

  ezStringBuilder sPath;

  for (ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezComponent>())
      continue;

    sPath.Set("ShapeIcons/", pRtti->GetTypeName(), ".dds");

    if (ezFileSystem::ExistsFile(sPath))
    {
      auto& shapeIconInfo = m_ShapeIconInfos[pRtti];
      shapeIconInfo.m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sPath);
      shapeIconInfo.m_pColorProperty = FindColorProperty(pRtti);
      shapeIconInfo.m_pColorGammaProperty = FindColorGammaProperty(pRtti);
    }
  }
}
