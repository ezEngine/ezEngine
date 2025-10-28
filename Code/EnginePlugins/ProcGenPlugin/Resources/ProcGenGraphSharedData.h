#pragma once

#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

namespace ezProcGenInternal
{
  class EZ_PROCGENPLUGIN_DLL CurveData : public ezReflectedClass
  {
    EZ_ADD_DYNAMIC_REFLECTION(CurveData, ezReflectedClass);

    ezDynamicArray<float> m_Samples;
    float m_fMinX = 0.0f;
    float m_fMaxX = 1.0f;

    void Save(ezStreamWriter& inout_stream) const;
    ezResult Load(ezStreamReader& inout_stream);
  };

  class EZ_PROCGENPLUGIN_DLL GraphSharedData : public GraphSharedDataBase
  {
  public:
    ezUInt32 AddTagSet(const ezTagSet& tagSet);
    ezUInt32 AddCurve(ezDynamicArray<float>&& samples, float fMinX, float fMaxX);

    const ezTagSet& GetTagSet(ezUInt32 uiIndex) const;
    const CurveData& GetCurve(ezUInt32 uiIndex) const;

    void Save(ezStreamWriter& inout_stream) const;
    ezResult Load(ezStreamReader& inout_stream);

  private:
    ezDynamicArray<ezTagSet> m_TagSets;
    ezDynamicArray<CurveData> m_Curves;
  };

} // namespace ezProcGenInternal
