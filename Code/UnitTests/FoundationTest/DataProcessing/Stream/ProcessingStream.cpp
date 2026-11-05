
#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Reflection/Reflection.h>

EZ_CREATE_SIMPLE_TEST_GROUP(DataProcessing);

// Zero initializer

class ZeroInitStreamProcessor : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ZeroInitStreamProcessor, ezProcessingStreamProcessor);

public:
  void SetStream(ezProcessingStream* pStream) { m_pStream = pStream; }

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override
  {
    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      ezMemoryUtils::ZeroFill<ezUInt8>(
        static_cast<ezUInt8*>(ezMemoryUtils::AddByteOffset(m_pStream->GetWritableData(), static_cast<std::ptrdiff_t>(i * m_pStream->GetElementStride()))),
        static_cast<size_t>(m_pStream->GetElementSize()));
    }
  }

  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStream = nullptr;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ZeroInitStreamProcessor, 1, ezRTTIDefaultAllocator<ZeroInitStreamProcessor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// Add processor

class AddOneStreamProcessor : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(AddOneStreamProcessor, ezProcessingStreamProcessor);

public:
  void SetStream(ezProcessingStream* pStream) { m_pStream = pStream; }

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}

  virtual void Process(ezUInt64 uiNumElements) override
  {
    ezProcessingStreamIterator<float> streamIterator(m_pStream, uiNumElements, 0);

    while (!streamIterator.HasReachedEnd())
    {
      streamIterator.Current() += 1.0f;

      streamIterator.Advance();
    }
  }

  ezProcessingStream* m_pStream = nullptr;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(AddOneStreamProcessor, 1, ezRTTIDefaultAllocator<AddOneStreamProcessor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_CREATE_SIMPLE_TEST(DataProcessing, ProcessingStreamMemory)
{
  const ezHashedString sFloat = ezMakeHashedString("Float");
  const ezHashedString sByte = ezMakeHashedString("Byte");
  const ezHashedString sVec4 = ezMakeHashedString("Vec4");
  const ezHashedString sNew = ezMakeHashedString("New");

  constexpr ezUInt32 uiNumElements = 32;
  constexpr ezUInt32 uiNumActive = 5;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Streams share one aligned block")
  {
    ezProcessingStreamGroup group;
    group.SetSize(uiNumElements);

    ezProcessingStream* pFloat = group.AddStream(sFloat, ezProcessingStream::DataType::Float);
    ezProcessingStream* pByte = group.AddStream(sByte, ezProcessingStream::DataType::Byte);
    ezProcessingStream* pVec4 = group.AddStream(sVec4, ezProcessingStream::DataType::Float4);

    EZ_TEST_BOOL(pFloat->GetData() == nullptr);

    group.Process();

    EZ_TEST_INT(pFloat->GetDataSize(), uiNumElements * 4);
    EZ_TEST_INT(pByte->GetDataSize(), uiNumElements);
    EZ_TEST_INT(pVec4->GetDataSize(), uiNumElements * 16);

    EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pFloat->GetData(), 16));
    EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pByte->GetData(), 16));
    EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pVec4->GetData(), 16));

    // 32 bytes of the byte stream already end on the required alignment
    EZ_TEST_BOOL(pByte->GetData() == ezMemoryUtils::AddByteOffset(pFloat->GetData(), uiNumElements * 4));
    EZ_TEST_BOOL(pVec4->GetData() == ezMemoryUtils::AddByteOffset(pByte->GetData(), uiNumElements));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Adding a stream keeps active elements")
  {
    ezProcessingStreamGroup group;
    group.SetSize(uiNumElements);

    ezProcessingStream* pFloat = group.AddStream(sFloat, ezProcessingStream::DataType::Float);
    ezProcessingStream* pVec4 = group.AddStream(sVec4, ezProcessingStream::DataType::Float4);

    group.InitializeElements(uiNumActive);
    group.Process();
    EZ_TEST_INT(group.GetNumActiveElements(), uiNumActive);

    for (ezUInt32 i = 0; i < uiNumActive; ++i)
    {
      pFloat->GetWritableData<float>()[i] = static_cast<float>(i);
      pVec4->GetWritableData<ezVec4>()[i] = ezVec4(static_cast<float>(i) * 2.0f);
    }

    const void* pOldData = pFloat->GetData();

    ezProcessingStream* pNew = group.AddStream(sNew, ezProcessingStream::DataType::Int);
    EZ_TEST_BOOL(pFloat->GetData() == pOldData);
    EZ_TEST_BOOL(pNew->GetData() == nullptr);

    group.Process();

    EZ_TEST_BOOL(pNew->GetData() != nullptr);
    EZ_TEST_INT(group.GetNumActiveElements(), uiNumActive);

    for (ezUInt32 i = 0; i < uiNumActive; ++i)
    {
      EZ_TEST_FLOAT(pFloat->GetData<float>()[i], static_cast<float>(i), 0.0f);
      EZ_TEST_VEC4(pVec4->GetData<ezVec4>()[i], ezVec4(static_cast<float>(i) * 2.0f), 0.0f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Removing a stream keeps the memory of the others")
  {
    ezProcessingStreamGroup group;
    group.SetSize(uiNumElements);

    group.AddStream(sFloat, ezProcessingStream::DataType::Float);
    ezProcessingStream* pVec4 = group.AddStream(sVec4, ezProcessingStream::DataType::Float4);

    group.InitializeElements(uiNumActive);
    group.Process();

    pVec4->GetWritableData<ezVec4>()[3] = ezVec4(3.0f);
    const void* pOldData = pVec4->GetData();

    group.RemoveStreamByName(sFloat);
    EZ_TEST_BOOL(group.GetStreamByName(sFloat) == nullptr);

    group.Process();

    EZ_TEST_BOOL(pVec4->GetData() == pOldData);
    EZ_TEST_VEC4(pVec4->GetData<ezVec4>()[3], ezVec4(3.0f), 0.0f);

    // a removed stream object can be reused, it must not keep its old data
    ezProcessingStream* pNew = group.AddStream(sNew, ezProcessingStream::DataType::Int);
    EZ_TEST_BOOL(pNew->GetData() == nullptr);
    EZ_TEST_BOOL(pNew->GetDataType() == ezProcessingStream::DataType::Int);
    EZ_TEST_INT(pNew->GetElementSize(), 4);
    EZ_TEST_INT(pNew->GetElementStride(), 4);

    group.Process();

    EZ_TEST_BOOL(pNew->GetData() != nullptr);
    EZ_TEST_VEC4(pVec4->GetData<ezVec4>()[3], ezVec4(3.0f), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reuse after Clear")
  {
    ezProcessingStreamGroup group;

    for (ezUInt32 uiCycle = 0; uiCycle < 3; ++uiCycle)
    {
      group.SetSize(uiNumElements);

      ezProcessingStream* pFloat = group.AddStream(sFloat, ezProcessingStream::DataType::Float);
      EZ_TEST_BOOL(pFloat->GetData() == nullptr);

      group.InitializeElements(uiNumActive);
      group.Process();

      EZ_TEST_INT(group.GetNumActiveElements(), uiNumActive);
      EZ_TEST_INT(pFloat->GetDataSize(), uiNumElements * 4);

      group.Clear();

      EZ_TEST_INT(group.GetNumElements(), 0);
      EZ_TEST_BOOL(group.GetStreamByName(sFloat) == nullptr);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "No elements")
  {
    ezProcessingStreamGroup group;
    ezProcessingStream* pFloat = group.AddStream(sFloat, ezProcessingStream::DataType::Float);

    group.Process();

    EZ_TEST_BOOL(pFloat->GetData() == nullptr);
    EZ_TEST_INT(pFloat->GetDataSize(), 0);
  }
}

EZ_CREATE_SIMPLE_TEST(DataProcessing, ProcessingStream)
{
  ezProcessingStreamGroup Group;
  ezProcessingStream* pStream1 = Group.AddStream(ezMakeHashedString("Stream1"), ezProcessingStream::DataType::Float);
  ezProcessingStream* pStream2 = Group.AddStream(ezMakeHashedString("Stream2"), ezProcessingStream::DataType::Float3);

  EZ_TEST_BOOL(pStream1 != nullptr);
  EZ_TEST_BOOL(pStream2 != nullptr);

  ZeroInitStreamProcessor* pSpawner1 = EZ_DEFAULT_NEW(ZeroInitStreamProcessor);
  ZeroInitStreamProcessor* pSpawner2 = EZ_DEFAULT_NEW(ZeroInitStreamProcessor);

  pSpawner1->SetStream(pStream1);
  pSpawner2->SetStream(pStream2);

  Group.AddProcessor(pSpawner1);
  Group.AddProcessor(pSpawner2);

  Group.SetSize(128);

  EZ_TEST_INT(Group.GetNumElements(), 128);
  EZ_TEST_INT(Group.GetNumActiveElements(), 0);

  Group.InitializeElements(3);

  Group.Process();

  EZ_TEST_INT(Group.GetNumActiveElements(), 3);


  {
    ezProcessingStreamIterator<float> stream1Iterator(pStream1, 3, 0);

    int iElementsVisited = 0;
    while (!stream1Iterator.HasReachedEnd())
    {
      EZ_TEST_FLOAT(stream1Iterator.Current(), 0.0f, 0.0f);

      stream1Iterator.Advance();
      iElementsVisited++;
    }

    EZ_TEST_INT(iElementsVisited, 3);
  }

  Group.InitializeElements(7);

  Group.Process();

  {
    ezProcessingStreamIterator<ezVec3> stream2Iterator(pStream2, Group.GetNumActiveElements(), 0);

    int iElementsVisited = 0;
    while (!stream2Iterator.HasReachedEnd())
    {
      EZ_TEST_FLOAT(stream2Iterator.Current().x, 0.0f, 0.0f);
      EZ_TEST_FLOAT(stream2Iterator.Current().y, 0.0f, 0.0f);
      EZ_TEST_FLOAT(stream2Iterator.Current().z, 0.0f, 0.0f);

      stream2Iterator.Advance();
      iElementsVisited++;
    }

    EZ_TEST_INT(iElementsVisited, 10);
  }

  EZ_TEST_INT(Group.GetHighestNumActiveElements(), 10);

  Group.RemoveElement(5);
  Group.RemoveElement(7);

  Group.Process();

  EZ_TEST_INT(Group.GetHighestNumActiveElements(), 10);
  EZ_TEST_INT(Group.GetNumActiveElements(), 8);

  AddOneStreamProcessor* pProcessor1 = EZ_DEFAULT_NEW(AddOneStreamProcessor);
  pProcessor1->SetStream(pStream1);

  Group.AddProcessor(pProcessor1);

  Group.Process();

  {
    ezProcessingStreamIterator<float> stream1Iterator(pStream1, Group.GetNumActiveElements(), 0);
    while (!stream1Iterator.HasReachedEnd())
    {
      EZ_TEST_FLOAT(stream1Iterator.Current(), 1.0f, 0.001f);

      stream1Iterator.Advance();
    }
  }

  Group.Process();

  {
    ezProcessingStreamIterator<float> stream1Iterator(pStream1, Group.GetNumActiveElements(), 0);
    while (!stream1Iterator.HasReachedEnd())
    {
      EZ_TEST_FLOAT(stream1Iterator.Current(), 2.0f, 0.001f);

      stream1Iterator.Advance();
    }
  }
}
