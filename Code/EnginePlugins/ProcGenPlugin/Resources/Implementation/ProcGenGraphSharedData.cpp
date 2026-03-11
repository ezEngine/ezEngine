#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

namespace ezProcGenInternal
{
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(CurveData, 1, ezRTTIDefaultAllocator<CurveData>)
  EZ_END_DYNAMIC_REFLECTED_TYPE;

  void CurveData::Save(ezStreamWriter& inout_stream) const
  {
    inout_stream.WriteArray(m_Samples).IgnoreResult();
    inout_stream << m_fMinX;
    inout_stream << m_fMaxX;
  }

  ezResult CurveData::Load(ezStreamReader& inout_stream)
  {
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Samples));
    inout_stream >> m_fMinX;
    inout_stream >> m_fMaxX;

    return EZ_SUCCESS;
  }

  ////////////////////////////////////////////////////////////////////////////

  ezUInt32 GraphSharedData::AddTagSet(const ezTagSet& tagSet)
  {
    ezUInt32 uiIndex = m_TagSets.IndexOf(tagSet);
    if (uiIndex == ezInvalidIndex)
    {
      uiIndex = m_TagSets.GetCount();
      m_TagSets.PushBack(tagSet);
    }
    return uiIndex;
  }

  ezUInt32 GraphSharedData::AddCurve(ezDynamicArray<float>&& samples, float fMinX, float fMaxX)
  {
    for (ezUInt32 uiIndex = 0; uiIndex < m_Curves.GetCount(); ++uiIndex)
    {
      const CurveData& existingCurve = m_Curves[uiIndex];
      if (existingCurve.m_Samples == samples && existingCurve.m_fMinX == fMinX && existingCurve.m_fMaxX == fMaxX)
      {
        return uiIndex;
      }
    }

    const ezUInt32 uiNewIndex = m_Curves.GetCount();
    CurveData& curve = m_Curves.ExpandAndGetRef();
    curve.m_Samples = std::move(samples);
    curve.m_fMinX = fMinX;
    curve.m_fMaxX = fMaxX;

    return uiNewIndex;
  }

  const ezTagSet& GraphSharedData::GetTagSet(ezUInt32 uiIndex) const
  {
    return m_TagSets[uiIndex];
  }

  const CurveData& GraphSharedData::GetCurve(ezUInt32 uiIndex) const
  {
    return m_Curves[uiIndex];
  }

  static ezTypeVersion s_GraphSharedDataVersion = 2;

  void GraphSharedData::Save(ezStreamWriter& inout_stream) const
  {
    inout_stream.WriteVersion(s_GraphSharedDataVersion);

    {
      const ezUInt32 uiCount = m_TagSets.GetCount();
      inout_stream << uiCount;

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets[i].Save(inout_stream);
      }
    }

    {
      const ezUInt32 uiCount = m_Curves.GetCount();
      inout_stream << uiCount;

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        m_Curves[i].Save(inout_stream);
      }
    }
  }

  ezResult GraphSharedData::Load(ezStreamReader& inout_stream)
  {
    auto version = inout_stream.ReadVersion(s_GraphSharedDataVersion);

    {
      ezUInt32 uiCount = 0;
      inout_stream >> uiCount;

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets.ExpandAndGetRef().Load(inout_stream, ezTagRegistry::GetGlobalRegistry());
      }
    }

    if (version >= 2)
    {
      ezUInt32 uiCount = 0;
      inout_stream >> uiCount;

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        CurveData curveData;
        EZ_SUCCEED_OR_RETURN(curveData.Load(inout_stream));
        m_Curves.PushBack(std::move(curveData));
      }
    }

    return EZ_SUCCESS;
  }

} // namespace ezProcGenInternal


EZ_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Resources_Implementation_ProcGenGraphSharedData);
