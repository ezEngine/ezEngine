#include <PCH.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>

class ezSomeSerializationObj
{
public:
  ezSomeSerializationObj(ezInt32 val)
  {
    m_iData = val;
  }

  ezInt32 m_iData;
};

class ezSerializationContextTest : public ezSerializationContext<ezSerializationContextTest>
{
public:

  void SetStream(ezStreamReaderBase& stream)
  {
    RegisterReaderStream(&stream);
  }

  void SetStream(ezStreamWriterBase& stream)
  {
    RegisterWriterStream(&stream);
  }

  void Read(ezStreamReaderBase& stream, ezSomeSerializationObj& obj)
  {
    stream >> obj.m_iData;
  }

  void Write(ezStreamWriterBase& stream, const ezSomeSerializationObj& obj)
  {
    stream << obj.m_iData;
  }
};

EZ_ADD_SERIALIZATION_CONTEXT_OPERATORS(ezSerializationContextTest, ezSomeSerializationObj&);


EZ_CREATE_SIMPLE_TEST(IO, ChunkStream)
{
  ezMemoryStreamStorage StreamStorage;

  ezMemoryStreamWriter MemoryWriter(&StreamStorage);
  ezMemoryStreamReader MemoryReader(&StreamStorage);

  ezSerializationContextTest SerialContext;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write Format")
  {
    ezChunkStreamWriter writer(MemoryWriter);

    SerialContext.SetStream(writer);

    writer.BeginStream();

    {
      writer.BeginChunk("Chunk1", 1);

      writer << (ezUInt32) 4;
      writer << (float) 5.6f;
      writer << (double) 7.8;
      writer << "nine";
      writer << ezVec3(10, 11.2f, 13.4f);
      writer << ezSomeSerializationObj(50);

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk2", 2);

      writer << "chunk 2 content";
      writer << ezSomeSerializationObj(60);

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk3", 3);

      writer << "chunk 3 content";
      writer << ezSomeSerializationObj(70);

      writer.EndChunk();
    }

    writer.EndStream();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read Format")
  {
    ezChunkStreamReader reader(MemoryReader);
    SerialContext.SetStream(reader);

    reader.BeginStream();

    // Chunk 1
    {
      EZ_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      EZ_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk1");
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 1);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 40);

      ezUInt32 i;
      float f;
      double d;
      ezString s;
      ezVec3 v;

      reader >> i;
      reader >> f;
      reader >> d;
      reader >> s;
      reader >> v;

      EZ_TEST_INT(i, 4);
      EZ_TEST_FLOAT(f, 5.6f, 0);
      EZ_TEST_DOUBLE(d, 7.8, 0);
      EZ_TEST_STRING(s.GetData(), "nine");
      EZ_TEST_VEC3(v, ezVec3(10, 11.2f, 13.4f), 0);

      EZ_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 4);
      reader.NextChunk();
    }

    // Chunk 2
    {
      EZ_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      EZ_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk2");
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 2);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 23);

      ezString s;
      ezSomeSerializationObj i(0);

      reader >> s;
      reader >> i;

      EZ_TEST_STRING(s.GetData(), "chunk 2 content");
      EZ_TEST_INT(i.m_iData, 60);

      EZ_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    // Chunk 3
    {
      EZ_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      EZ_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk3");
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 3);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 23);

      ezString s;

      reader >> s;

      EZ_TEST_STRING(s.GetData(), "chunk 3 content");

      EZ_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 4);
      reader.NextChunk();
    }

    EZ_TEST_BOOL(!reader.GetCurrentChunk().m_bValid);

    reader.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::SkipToEnd);
    reader.EndStream();

    ezUInt8 Temp[1024];
    EZ_TEST_INT(MemoryReader.ReadBytes(Temp, 1024), 0); // nothing left to read
  }
}





