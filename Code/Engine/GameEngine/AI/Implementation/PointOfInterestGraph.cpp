#include <PCH.h>
#include <GameEngine/AI/PointOfInterestGraph.h>

struct ezDummyPointType
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 value;
};

static void CompileDummy()
{
  ezPointOfInterestGraph<ezDummyPointType> graph;
  graph.Initialize(ezVec3::ZeroVector(), ezVec3::ZeroVector());
  auto& pt = graph.AddPoint(ezVec3::ZeroVector());

  ezDynamicArray<ezUInt32> points;
  graph.FindPointsOfInterest(ezVec3::ZeroVector(), 0, points);
}