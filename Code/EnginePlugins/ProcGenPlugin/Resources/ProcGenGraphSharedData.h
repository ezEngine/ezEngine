#pragma once

#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

namespace ezProcGenInternal
{

  class EZ_PROCGENPLUGIN_DLL GraphSharedData : public GraphSharedDataBase
  {
  public:
    ezUInt32 AddTagSet(const ezTagSet& tagSet);

    void Save(ezStreamWriter& stream) const;
    ezResult Load(ezStreamReader& stream);

  private:
    ezDynamicArray<ezTagSet> m_TagSets;
  };

} // namespace ezProcGenInternal
