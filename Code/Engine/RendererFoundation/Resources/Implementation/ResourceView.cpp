#include <PCH.h>

#include <RendererFoundation/Resources/ResourceView.h>


ezGALResourceView::ezGALResourceView(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& description)
    : ezGALObject(description)
    , m_pResource(pResource)
{
  EZ_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

ezGALResourceView::~ezGALResourceView() {}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceView);
