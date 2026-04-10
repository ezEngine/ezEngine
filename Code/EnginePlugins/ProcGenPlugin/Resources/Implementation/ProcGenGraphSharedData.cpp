#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

namespace ezProcGenInternal
{
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

  ezUInt32 GraphSharedData::AddCurve(ezSampledCurve1D&& curve)
  {
    ezUInt32 uiIndex = m_Curves.IndexOf(curve);
    if (uiIndex == ezInvalidIndex)
    {
      uiIndex = m_Curves.GetCount();
      m_Curves.PushBack(std::move(curve));
    }

    return uiIndex;
  }

  const ezTagSet& GraphSharedData::GetTagSet(ezUInt32 uiIndex) const
  {
    return m_TagSets[uiIndex];
  }

  const ezSampledCurve1D& GraphSharedData::GetCurve(ezUInt32 uiIndex) const
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
        ezSampledCurve1D curveData;
        EZ_SUCCEED_OR_RETURN(curveData.Load(inout_stream));
        m_Curves.PushBack(std::move(curveData));
      }
    }

    return EZ_SUCCESS;
  }

} // namespace ezProcGenInternal


EZ_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Resources_Implementation_ProcGenGraphSharedData);
