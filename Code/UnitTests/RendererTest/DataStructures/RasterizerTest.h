#pragma once

#include "../TestClass/TestClass.h"
#include <Foundation/Containers/Deque.h>
#include <RendererCore/Rasterizer/RasterizerView.h>

class ezRasterizerTest : public ezGraphicsTest
{
public:
  ezRasterizerTest() = default;

  virtual const char* GetTestName() const override { return "Rasterizer"; }

private:
  /// The city scenes that every rasterizer implementation is benchmarked with.
  ///
  /// The sub-test identifier is 'implementation * Scene::Count + scene'.
  enum class Scene : ezInt32
  {
    SideFacade,   ///< City seen from the side with a facade in front, also runs the AABB shrink search and functional checks.
    BehindFacade, ///< Camera right behind the facade, the facade fills the whole view and hides every building.
    TopAngled,    ///< City seen from above at an angle, most buildings are visible and the landscape fills the rest.
    SideSky,      ///< Horizontal view over the city, the upper half is empty sky.
    Count
  };

  virtual void SetupSubTests() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;
  void AddSceneSubTests(ezRasterizerImplementation implementation);

  ezDeque<ezString> m_SubTestNames;
};
