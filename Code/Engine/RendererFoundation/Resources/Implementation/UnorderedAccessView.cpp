#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

ezGALUnorderedAccessView::ezGALUnorderedAccessView(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& description)
  : ezGALObject(description)
  , m_pResource(pResource)
{
  EZ_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

ezGALUnorderedAccessView::~ezGALUnorderedAccessView()
{
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceView);
