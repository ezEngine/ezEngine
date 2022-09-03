#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Serialization/SerializeTree.h>

namespace Kraut
{
  void Serializer::Serialize(aeStreamOut& stream) const
  {
    const char* szSignature = "<Kraut>";
    stream.Write(szSignature, sizeof(char) * 7);

    const aeUInt8 uiVersion = 2;
    stream << uiVersion;

    m_pTreeStructure->Serialize(stream);

    const aeUInt8 uiNumLods = 5;
    stream << uiNumLods;
    for (aeUInt32 i = 0; i < uiNumLods; ++i)
    {
      m_LODs[i]->Serialize(stream);
    }
  }

  bool Deserializer::Deserialize(aeStreamIn& stream)
  {
    char szSignature[8] = "";
    stream.Read(szSignature, sizeof(char) * 7);
    szSignature[7] = '\0';

    if (!aeStringFunctions::CompareEqual(szSignature, "<Kraut>"))
      return false;

    aeUInt8 uiVersion = 0;
    stream >> uiVersion;

    if (uiVersion != 1 && uiVersion != 2)
      return false;

    m_pTreeStructure->Deserialize(stream);

    aeUInt8 uiNumLODs = 0;
    stream >> uiNumLODs;

    AE_CHECK_DEV(uiNumLODs == 5, "Invalid number of LODs");

    for (aeUInt32 i = 0; i < uiNumLODs; ++i)
    {
      m_LODs[i]->Deserialize(stream);
    }

    if (uiVersion == 1)
    {
      aeUInt32 uiNumSeeds = 0;
      stream >> uiNumSeeds;
    }

    return true;
  }

} // namespace Kraut
