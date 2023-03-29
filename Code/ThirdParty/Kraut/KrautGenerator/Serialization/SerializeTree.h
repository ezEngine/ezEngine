#pragma once

#include <KrautFoundation/Containers/HybridArray.h>
#include <KrautFoundation/Streams/Streams.h>
#include <KrautGenerator/Description/DescriptionEnums.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct TreeStructureDesc;
  struct LodDesc;

  struct KRAUT_DLL Serializer
  {
    const Kraut::TreeStructureDesc* m_pTreeStructure = nullptr;
    const Kraut::LodDesc* m_LODs[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};

    void Serialize(aeStreamOut& stream) const;
  };

  struct KRAUT_DLL Deserializer
  {
    Kraut::TreeStructureDesc* m_pTreeStructure = nullptr;
    Kraut::LodDesc* m_LODs[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};

    bool Deserialize(aeStreamIn& stream);
  };

} // namespace Kraut
