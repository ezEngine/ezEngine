#include <RendererTestPCH.h>

#include <RendererCore/Textures/TextureUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN("RendererTest", "Renderer Tests")
{
  ezTextureUtils::s_bForceFullQualityAlways = true; // never allow to use low-res textures
}
EZ_TESTFRAMEWORK_ENTRY_POINT_END()
