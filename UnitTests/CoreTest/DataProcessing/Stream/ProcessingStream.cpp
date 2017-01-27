
#include <PCH.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <CoreUtils/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>

EZ_CREATE_SIMPLE_TEST_GROUP(DataProcessing);

// Add processor

class AddOneStreamProcessor : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(AddOneStreamProcessor, ezProcessingStreamProcessor);

public:

  AddOneStreamProcessor()
    : m_pStream(nullptr)
  {}

  void SetStreamName(ezHashedString StreamName)
  {
    m_StreamName = StreamName;
  }

protected:

  virtual ezResult UpdateStreamBindings() override
  {
    m_pStream = m_pStreamGroup->GetStreamByName(m_StreamName);

    return m_pStream ? EZ_SUCCESS : EZ_FAILURE;
  }

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override { }

  virtual void Process(ezUInt64 uiNumElements) override
  {
    ezProcessingStreamIterator<float> streamIterator(m_pStream, uiNumElements, 0);

    while (!streamIterator.HasReachedEnd())
    {
      streamIterator.Current() += 1.0f;

      streamIterator.Advance();
    }
  }

  ezHashedString m_StreamName;
  ezProcessingStream* m_pStream;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(AddOneStreamProcessor, 1, ezRTTIDefaultAllocator<AddOneStreamProcessor>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_CREATE_SIMPLE_TEST(DataProcessing, ProcessingStream)
{
  ezProcessingStreamGroup Group;
  ezProcessingStream* pStream1 = Group.AddStream("Stream1", ezProcessingStream::DataType::Float);
  ezProcessingStream* pStream2 = Group.AddStream("Stream2", ezProcessingStream::DataType::Float3);

  EZ_TEST_BOOL(pStream1 != nullptr);
  EZ_TEST_BOOL(pStream2 != nullptr);

  ezProcessingStreamSpawnerZeroInitialized* pSpawner1 = EZ_DEFAULT_NEW(ezProcessingStreamSpawnerZeroInitialized);
  ezProcessingStreamSpawnerZeroInitialized* pSpawner2 = EZ_DEFAULT_NEW(ezProcessingStreamSpawnerZeroInitialized);

  pSpawner1->SetStreamName(pStream1->GetName());
  pSpawner2->SetStreamName(pStream2->GetName());

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
  pProcessor1->SetStreamName(pStream1->GetName());

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
