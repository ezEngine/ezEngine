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
    EZ_MEMBER_PROPERTY("Max Screen Size", m_fMaxScreenSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(64.0f), new ezSuffixAttribute(" px")),
    EZ_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


ezEditorShapeIconsExtractor::ezEditorShapeIconsExtractor()
{
  m_fSize = 1.0f;
  m_fMaxScreenSize = 64.0f;
  m_pSceneContext = nullptr;

  LoadShapeIconTextures();
}

ezEditorShapeIconsExtractor::~ezEditorShapeIconsExtractor()
{
}

void ezEditorShapeIconsExtractor::Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  EZ_LOCK(view.GetWorld()->GetReadMarker());

  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
  {
    const ezGameObject* pObject = it;

    ExtractShapeIcon(pObject, view, pExtractedRenderData, ezDefaultRenderDataCategories::LitOpaque);
  }

  if (m_pSceneContext != nullptr)
  {
    auto objects = m_pSceneContext->GetSelectionWithChildren();

    for (const auto& hObject : objects)
    {
      ezGameObject* pObject = nullptr;
      if (view.GetWorld()->TryGetObject(hObject, pObject))
      {
        ExtractShapeIcon(pObject, view, pExtractedRenderData, ezDefaultRenderDataCategories::Selection);
      }
    }
  }
}

void ezEditorShapeIconsExtractor::ExtractShapeIcon(const ezGameObject* pObject, const ezView& view, ezExtractedRenderData* pExtractedRenderData, ezRenderData::Category category)
{
  static const ezTag* tagHidden = ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden");
  static const ezTag* tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  if (pObject->GetTags().IsSet(*tagEditor) || pObject->GetTags().IsSet(*tagHidden))
    return;

  if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
    return;

  if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
    return;

  if (pObject->GetComponents().IsEmpty())
    return;

  const ezComponent* pComponent = pObject->GetComponents()[0];
  const ezRTTI* pRtti = pComponent->GetDynamicRTTI();

  ezTextureResourceHandle hTexture;
  if (m_ShapeIcons.TryGetValue(pRtti, hTexture))
  {
    const ezUInt32 uiTextureIDHash = hTexture.GetResourceIDHash();
    ezSpriteRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezSpriteRenderData>(pObject, uiTextureIDHash);
    {
      pRenderData->m_GlobalTransform = pObject->GetGlobalTransform();
      pRenderData->m_GlobalBounds = pObject->GetGlobalBounds();
      pRenderData->m_hTexture = hTexture;
      pRenderData->m_fSize = m_fSize;
      pRenderData->m_fMaxScreenSize = m_fMaxScreenSize;
      pRenderData->m_color = ezColor::White;
      pRenderData->m_texCoordScale = ezVec2(1.0f);
      pRenderData->m_texCoordOffset = ezVec2(0.0f);
      pRenderData->m_uiEditorPickingID = pComponent->GetEditorPickingID();
    }

    pExtractedRenderData->AddRenderData(pRenderData, category, uiTextureIDHash);
  }
}

void ezEditorShapeIconsExtractor::LoadShapeIconTextures()
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
      m_ShapeIcons[pRtti] = ezResourceManager::LoadResource<ezTextureResource>(sPath);
    }
  }
}
