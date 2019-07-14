#include <GameEngineTestPCH.h>

#include <RendererCore/Textures/TextureUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN("GameEngineTest", "GameEngine Tests")
{
  ezTextureUtils::s_bForceFullQualityAlways = true; // never allow to use low-res textures
  ezTestFramework::GetInstance()->SetTestTimeout(1000 * 60 * 10);
  ezTestFramework::s_bCallstackOnAssert = true;
}
EZ_TESTFRAMEWORK_ENTRY_POINT_END()
