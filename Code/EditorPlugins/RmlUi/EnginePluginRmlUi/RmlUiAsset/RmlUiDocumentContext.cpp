#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiDocumentContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiViewContext.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiDocumentContext, 1, ezRTTIDefaultAllocator<ezRmlUiDocumentContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "RmlUi"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRmlUiDocumentContext::ezRmlUiDocumentContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

ezRmlUiDocumentContext::~ezRmlUiDocumentContext() = default;

void ezRmlUiDocumentContext::OnInitialize()
{
  auto pWorld = m_pWorld;
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

ezEngineProcessViewContext* ezRmlUiDocumentContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezRmlUiViewContext, this);
}

void ezRmlUiDocumentContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezRmlUiDocumentContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  EZ_LOCK(m_pMainObject->GetWorld()->GetWriteMarker());

  m_pMainObject->UpdateLocalBounds();
  ezBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  ezRmlUiViewContext* pMeshViewContext = static_cast<ezRmlUiViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}
