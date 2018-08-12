#include <PCH.h>

#include <RendererFoundation/Resources/Query.h>

ezGALQuery::ezGALQuery(const ezGALQueryCreationDescription& Description)
    : ezGALResource<ezGALQueryCreationDescription>(Description)
    , m_bStarted(false)
{
}

ezGALQuery::~ezGALQuery() {}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Query);
