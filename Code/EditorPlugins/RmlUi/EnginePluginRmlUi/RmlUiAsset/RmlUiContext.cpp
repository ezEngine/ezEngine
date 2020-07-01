#include <EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiView.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiContext, 1, ezRTTIDefaultAllocator<ezRmlUiContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "RmlUi"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRmlUiContext::ezRmlUiContext() = default;

void ezRmlUiContext::OnInitialize()
{
  auto pWorld = m_pWorld.Borrow();
  EZ_LOCK(pWorld->GetWriteMarker());

  // Preview object
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("RmlUiPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    ezRmlUiCanvas2DComponent* pComponent = nullptr;
    ezRmlUiCanvas2DComponent::CreateComponent(m_pMainObject, pComponent);

    ezStringBuilder sResourceGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sResourceGuid);
    m_hMainResource = ezResourceManager::LoadResource<ezRmlUiResource>(sResourceGuid);

    pComponent->SetRmlResource(m_hMainResource);
  }
}

ezEngineProcessViewContext* ezRmlUiContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezRmlUiViewContext, this);
}

void ezRmlUiContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezRmlUiContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  m_pMainObject->UpdateLocalBounds();
  ezBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  ezRmlUiViewContext* pMeshViewContext = static_cast<ezRmlUiViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}
