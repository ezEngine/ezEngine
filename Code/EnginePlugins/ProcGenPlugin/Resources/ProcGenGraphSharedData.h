#pragma once

#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

namespace ezProcGenInternal
{
  class EZ_PROCGENPLUGIN_DLL GraphSharedData : public GraphSharedDataBase
  {
  public:
    ezUInt32 AddTagSet(const ezTagSet& tagSet);
    ezUInt32 AddCurve(ezSampledCurve1D&& curve);

    const ezTagSet& GetTagSet(ezUInt32 uiIndex) const;
    const ezSampledCurve1D& GetCurve(ezUInt32 uiIndex) const;

    void Save(ezStreamWriter& inout_stream) const;
    ezResult Load(ezStreamReader& inout_stream);

  private:
    ezDynamicArray<ezTagSet> m_TagSets;
    ezDynamicArray<ezSampledCurve1D> m_Curves;
  };

} // namespace ezProcGenInternal
