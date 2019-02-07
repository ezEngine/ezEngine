#include <PCH.h>

#include <Foundation/Containers/HashTable.h>
#include <ProceduralPlacementPlugin/ProceduralPlacementPluginDLL.h>

namespace ezPPInternal
{
  static Pattern::Point s_BayerPoints[64];

  static ezHashTable<ezUInt32, Pattern, ezHashHelper<ezUInt32>, ezStaticAllocatorWrapper> s_Patterns;

  bool FillPatterns()
  {
    // generate bayer pattern
    const ezUInt32 M = 3;
    const ezUInt32 n = 1 << M;

    for (ezUInt32 y = 0; y < n; ++y)
    {
      for (ezUInt32 x = 0; x < n; ++x)
      {
        ezUInt32 v = 0, mask = M - 1, xc = x ^ y, yc = y;
        for (ezUInt32 bit = 0; bit < 2 * M; --mask)
        {
          v |= ((yc >> mask) & 1) << bit++;
          v |= ((xc >> mask) & 1) << bit++;
        }

        auto& point = s_BayerPoints[y * n + x];
        point.m_Coordinates.Set(x + 0.5f, y + 0.5f);
        point.m_fThreshold = (v + 1.0f) / (n * n);
      }
    }

    Pattern bayerPattern;
    bayerPattern.m_Points = ezMakeArrayPtr(s_BayerPoints);
    bayerPattern.m_fSize = (float)n;

    s_Patterns.Insert(ezTempHashedString("Bayer").GetHash(), bayerPattern);

    return true;
  }

  static bool s_bFillPatternsDummy = FillPatterns();

  Pattern* GetPattern(ezTempHashedString sName) { return s_Patterns.GetValue(sName.GetHash()); }
}
