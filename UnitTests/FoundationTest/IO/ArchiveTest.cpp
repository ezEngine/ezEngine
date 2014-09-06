#include <PCH.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/Archive.h>
#include <Foundation/IO/MemoryStream.h>

EZ_CREATE_SIMPLE_TEST(IO, Archive)
{
  ezMemoryStreamStorage StreamStorage;

  ezMemoryStreamWriter MemoryWriter(&StreamStorage);
  ezMemoryStreamReader MemoryReader(&StreamStorage);

  ezVec3 TempWrite(23, 42, 88);
  ezVec3* TempRead = &TempWrite;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write Format")
  {
    ezArchiveWriter writer(MemoryWriter);

    writer.BeginStream();

    {
      writer.BeginChunk("Chunk1", 1);

      writer.WriteObjectReference(&TempWrite);

      ezVec3 v1(1, 2, 3);
      writer.BeginTypedObject(ezGetStaticRTTI<ezVec3>(), &v1);
      writer << v1;
      writer.EndTypedObject();

      writer.EndChunk();
    }

    writer.EndStream();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read Format")
  {
    ezArchiveReader reader(MemoryReader);

    reader.BeginStream();

    {
      EZ_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      EZ_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk1");
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 1);

      const ezUInt32 uiObjectID = reader.ReadObjectReference((void**) &TempRead);

      EZ_TEST_BOOL(TempRead == nullptr);

      reader.SetObjectReferenceMapping(uiObjectID, &TempWrite);

      //ezVec3 v1(1, 2, 3);
      //writer.BeginTypedObject(ezGetStaticRTTI<ezVec3>(), &v1);
      //writer << v1;
      //writer.EndTypedObject();

      reader.NextChunk();
    }

    EZ_TEST_BOOL(!reader.GetCurrentChunk().m_bValid);

    EZ_TEST_BOOL(TempRead == nullptr);

    reader.EndStream();

    EZ_TEST_BOOL(TempRead == &TempWrite);
  }

  int i = 0;
}





