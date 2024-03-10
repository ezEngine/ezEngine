#include <RecastPlugin/RecastPluginPCH.h>

#include <RecastPlugin/Components/PointOfInterestGraph.h>

struct ezDummyPointType
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 value;
};

void CompileDummy()
{
  ezPointOfInterestGraph<ezDummyPointType> graph;
  graph.Initialize(ezVec3::MakeZero(), ezVec3::MakeZero());
  auto& pt = graph.AddPoint(ezVec3::MakeZero());

  ezDynamicArray<ezUInt32> points;
  graph.FindPointsOfInterest(ezVec3::MakeZero(), 0, points);
}
