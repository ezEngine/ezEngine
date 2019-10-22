#include <ProcGenPluginPCH.h>

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

  static ezTypeVersion s_GraphSharedDataVersion = 1;

  void GraphSharedData::Save(ezStreamWriter& stream) const
  {
    stream.WriteVersion(s_GraphSharedDataVersion);

    {
      const ezUInt32 uiCount = m_TagSets.GetCount();
      stream << uiCount;

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets[i].Save(stream);
      }
    }
  }

  ezResult GraphSharedData::Load(ezStreamReader& stream)
  {
    auto version = stream.ReadVersion(s_GraphSharedDataVersion);

    {
      ezUInt32 uiCount = 0;
      stream >> uiCount;
      
      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets.ExpandAndGetRef().Load(stream, ezTagRegistry::GetGlobalRegistry());
      }
    }

    return EZ_SUCCESS;
  }

} // namespace ezProcGenInternal
