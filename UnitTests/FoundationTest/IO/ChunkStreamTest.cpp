#include <PCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>

EZ_CREATE_SIMPLE_TEST(IO, ChunkStream)
{
  ezMemoryStreamStorage StreamStorage;

  ezMemoryStreamWriter MemoryWriter(&StreamStorage);
  ezMemoryStreamReader MemoryReader(&StreamStorage);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write Format")
  {
    ezChunkStreamWriter writer(MemoryWriter);

    writer.BeginStream();

    {
      writer.BeginChunk("Chunk1", 1);

      writer << (ezUInt32)4;
      writer << (float)5.6f;
      writer << (double)7.8;
      writer << "nine";
      writer << ezVec3(10, 11.2f, 13.4f);

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk2", 2);

      writer << "chunk 2 content";

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk3", 3);

      writer << "chunk 3 content";

      writer.EndChunk();
    }

    writer.EndStream();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read Format")
  {
    ezChunkStreamReader reader(MemoryReader);

    reader.BeginStream();

    // Chunk 1
    {
      EZ_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      EZ_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk1");
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 1);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 36);

      ezUInt32 i;
      float f;
      double d;
      ezString s;

      reader >> i;
      reader >> f;
      reader >> d;
      reader >> s;

      EZ_TEST_INT(i, 4);
      EZ_TEST_FLOAT(f, 5.6f, 0);
      EZ_TEST_DOUBLE(d, 7.8, 0);
      EZ_TEST_STRING(s.GetData(), "nine");

      EZ_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 12);
      reader.NextChunk();
    }

    // Chunk 2
    {
      EZ_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      EZ_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk2");
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 2);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 19);

      ezString s;

      reader >> s;

      EZ_TEST_STRING(s.GetData(), "chunk 2 content");

      EZ_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    // Chunk 3
    {
      EZ_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      EZ_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk3");
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 3);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 19);

      ezString s;

      reader >> s;

      EZ_TEST_STRING(s.GetData(), "chunk 3 content");

      EZ_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    EZ_TEST_BOOL(!reader.GetCurrentChunk().m_bValid);

    reader.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::SkipToEnd);
    reader.EndStream();

    ezUInt8 Temp[1024];
    EZ_TEST_INT(MemoryReader.ReadBytes(Temp, 1024), 0); // nothing left to read
  }
}
