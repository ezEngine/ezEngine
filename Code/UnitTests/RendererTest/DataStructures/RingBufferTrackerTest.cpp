#include <RendererTest/RendererTestPCH.h>

#include <RendererFoundation/Utils/RingBufferTracker.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

EZ_CREATE_SIMPLE_RENDERER_TEST_GROUP(DataStructures)

EZ_CREATE_SIMPLE_RENDERER_TEST(DataStructures, RingBufferTracker)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Allocate entire buffer")
  {
    ezRingBufferTracker tracker(64, 65472);
    EZ_TEST_INT(tracker.GetTotalMemory(), 65472);
    EZ_TEST_INT(tracker.GetUsedMemory(), 0);
    EZ_TEST_INT(tracker.GetFreeMemory(), 65472);

    ezUInt32 uiOffset = 0;
    EZ_TEST_RESULT(tracker.Allocate(65472, 1, uiOffset));
    EZ_TEST_INT(uiOffset, 0);
    EZ_TEST_INT(tracker.GetTotalMemory(), 65472);
    EZ_TEST_INT(tracker.GetUsedMemory(), 65472);
    EZ_TEST_INT(tracker.GetFreeMemory(), 0);
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Allocate, submit, free")
  {
    ezRingBufferTracker tracker(64, 1024);
    ezUInt32 uiOffset = 0;
    EZ_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    EZ_TEST_INT(uiOffset, 0);
    EZ_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    EZ_TEST_INT(uiOffset, 256);
    EZ_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    EZ_TEST_INT(uiOffset, 512);
    EZ_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    EZ_TEST_INT(uiOffset, 768);
    EZ_TEST_BOOL(tracker.Allocate(256, 1, uiOffset).Failed());
    EZ_TEST_INT(tracker.GetFreeMemory(), 0);
    EZ_TEST_INT(tracker.GetUsedMemory(), 1024);

    ezHybridArray<ezRingBufferTracker::FrameData, 2> frames;
    EZ_TEST_RESULT(tracker.SubmitFrame(1, frames));
    EZ_TEST_INT(frames.GetCount(), 1);
    EZ_TEST_INT(frames[0].m_uiFrame, 1);
    EZ_TEST_INT(frames[0].m_uiStartOffset, 0);
    EZ_TEST_INT(frames[0].m_uiSize, 1024);

    tracker.Free(1);
    EZ_TEST_INT(tracker.GetFreeMemory(), 1024);
    EZ_TEST_INT(tracker.GetUsedMemory(), 0);
    EZ_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    EZ_TEST_INT(uiOffset, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Loop through the ringbuffer")
  {
    ezRingBufferTracker tracker(64, 1024);
    ezUInt32 uiOffset = 0;
    for (size_t i = 4; i < 20; i++)
    {
      EZ_TEST_INT(tracker.GetTotalMemory(), 1024);
      tracker.Free(i - 4);
      EZ_TEST_RESULT(tracker.Allocate(256, i, uiOffset));
      EZ_TEST_INT(uiOffset, (i * 256) % 1024);
      ezHybridArray<ezRingBufferTracker::FrameData, 2> frames;
      EZ_TEST_RESULT(tracker.SubmitFrame(i, frames));
      EZ_TEST_INT(frames.GetCount(), 1);
      EZ_TEST_INT(frames[0].m_uiFrame, i);
      EZ_TEST_INT(frames[0].m_uiStartOffset, (i * 256) % 1024);
      EZ_TEST_INT(frames[0].m_uiSize, 256);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Buffer wrap around")
  {
    ezRingBufferTracker tracker(2, 1000);
    ezUInt32 uiOffset = 0;

    // Frame 1: Move a bit forward so we are at the end of the buffer in the next frame.
    EZ_TEST_RESULT(tracker.Allocate(800, 1, uiOffset));
    EZ_TEST_INT(uiOffset, 0);
    EZ_TEST_INT(tracker.GetUsedMemory(), 800);
    ezHybridArray<ezRingBufferTracker::FrameData, 2> frames;
    EZ_TEST_RESULT(tracker.SubmitFrame(1, frames));
    EZ_TEST_INT(frames.GetCount(), 1);
    EZ_TEST_INT(frames[0].m_uiFrame, 1);
    EZ_TEST_INT(frames[0].m_uiStartOffset, 0);
    EZ_TEST_INT(frames[0].m_uiSize, 800);
    tracker.Free(1);
    EZ_TEST_INT(tracker.GetUsedMemory(), 0);

    // Frame 2: We allocate memory and hit the end point exactly and then do another allocation.
    EZ_TEST_RESULT(tracker.Allocate(200, 2, uiOffset));
    EZ_TEST_INT(uiOffset, 800);
    EZ_TEST_INT(tracker.GetUsedMemory(), 200);

    // This allocation wraps around now.
    EZ_TEST_RESULT(tracker.Allocate(200, 2, uiOffset));
    EZ_TEST_INT(uiOffset, 0);
    EZ_TEST_INT(tracker.GetUsedMemory(), 400);

    EZ_TEST_RESULT(tracker.SubmitFrame(2, frames));
    EZ_TEST_INT(frames.GetCount(), 2);

    EZ_TEST_INT(frames[0].m_uiFrame, 2);
    EZ_TEST_INT(frames[0].m_uiStartOffset, 800);
    EZ_TEST_INT(frames[0].m_uiSize, 200);

    EZ_TEST_INT(frames[1].m_uiFrame, 2);
    EZ_TEST_INT(frames[1].m_uiStartOffset, 0);
    EZ_TEST_INT(frames[1].m_uiSize, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Allocation skip due to reaching the end of the buffer")
  {
    ezRingBufferTracker tracker(2, 1000);
    ezUInt32 uiOffset = 0;

    // Frame 1: Move a bit forward so we are at the end of the buffer in the next frame.
    EZ_TEST_RESULT(tracker.Allocate(800, 1, uiOffset));
    EZ_TEST_INT(uiOffset, 0);
    EZ_TEST_INT(tracker.GetUsedMemory(), 800);
    ezHybridArray<ezRingBufferTracker::FrameData, 2> frames;
    EZ_TEST_RESULT(tracker.SubmitFrame(1, frames));
    EZ_TEST_INT(frames.GetCount(), 1);
    EZ_TEST_INT(frames[0].m_uiFrame, 1);
    EZ_TEST_INT(frames[0].m_uiStartOffset, 0);
    EZ_TEST_INT(frames[0].m_uiSize, 800);
    tracker.Free(1);
    EZ_TEST_INT(tracker.GetUsedMemory(), 0);

    // Frame 2: We allocate memory but do not hit the end point yet.
    EZ_TEST_RESULT(tracker.Allocate(100, 2, uiOffset));
    EZ_TEST_INT(uiOffset, 800);
    EZ_TEST_INT(tracker.GetUsedMemory(), 100);

    // We allocate near the end of the buffer but it doesn't fit so 100 bytes are wasted as we skip to the start again.
    EZ_TEST_RESULT(tracker.Allocate(200, 2, uiOffset));
    EZ_TEST_INT(uiOffset, 0);
    EZ_TEST_INT(tracker.GetUsedMemory(), 400);

    EZ_TEST_RESULT(tracker.SubmitFrame(2, frames));
    EZ_TEST_INT(frames.GetCount(), 2);

    EZ_TEST_INT(frames[0].m_uiFrame, 2);
    EZ_TEST_INT(frames[0].m_uiStartOffset, 800);
    EZ_TEST_INT(frames[0].m_uiSize, 200);

    EZ_TEST_INT(frames[1].m_uiFrame, 2);
    EZ_TEST_INT(frames[1].m_uiStartOffset, 0);
    EZ_TEST_INT(frames[1].m_uiSize, 200);
  }
}
