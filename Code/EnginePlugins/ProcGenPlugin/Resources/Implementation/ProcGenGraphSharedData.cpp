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

  const ezTagSet& GraphSharedData::GetTagSet(ezUInt32 uiIndex) const
  {
    return m_TagSets[uiIndex];
  }

  static ezTypeVersion s_GraphSharedDataVersion = 1;

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
  }

  ezResult GraphSharedData::Load(ezStreamReader& inout_stream)
  {
    auto version = inout_stream.ReadVersion(s_GraphSharedDataVersion);
    EZ_IGNORE_UNUSED(version);

    {
      ezUInt32 uiCount = 0;
      inout_stream >> uiCount;

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets.ExpandAndGetRef().Load(inout_stream, ezTagRegistry::GetGlobalRegistry());
      }
    }

    return EZ_SUCCESS;
  }

} // namespace ezProcGenInternal
