#pragma once

#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

namespace ezProcGenInternal
{

  class EZ_PROCGENPLUGIN_DLL GraphSharedData : public GraphSharedDataBase
  {
  public:
    ezUInt32 AddTagSet(const ezTagSet& tagSet);

    const ezTagSet& GetTagSet(ezUInt32 uiIndex) const;

    void Save(ezStreamWriter& inout_stream) const;
    ezResult Load(ezStreamReader& inout_stream);

  private:
    ezDynamicArray<ezTagSet> m_TagSets;
  };

} // namespace ezProcGenInternal
