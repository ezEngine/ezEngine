#include <GameEngineTest/GameEngineTestPCH.h>

#include "ComponentSerializationTest.h"

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Reflection.h>

namespace ComponentSerializationTestDetail
{
  /// Message of the mismatch that ezWorldReader reported during the current round trip, empty if it
  /// did not report one.
  static ezStringBuilder g_sReportedFailure;
  static ezAssertHandler g_PreviousAssertHandler = nullptr;

  /// Catches the failure that ezWorldReader::InstantiationContext::DeserializeComponents() reports
  /// when a component type read a different number of bytes than were stored for it.
  ///
  /// The report goes through the assert handler, which the test framework normally turns into a
  /// failed test and a debug break. Routing it here instead keeps the test running so that all
  /// remaining component types are still checked, and turns the message into a regular test failure.
  static bool SerializationAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
  {
    g_sReportedFailure = szAssertMsg;

    // Everything that is not the size mismatch is a real problem and has to reach the normal handler.
    if (g_sReportedFailure.FindSubString("deserialized") == nullptr)
    {
      if (g_PreviousAssertHandler != nullptr)
        return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

      return true;
    }

    // Do not break into the debugger, the caller turns this into a test failure.
    return false;
  }

  enum class Result
  {
    Ok,
    Skipped,     ///< No component of this type could be created, nothing was tested.
    SizeMismatch ///< Serialize and Deserialize disagree about the amount of data.
  };

  /// Serializes a single component of the given type into a world of its own and reads it back.
  ///
  /// ezWorldReader compares, per component type, how many bytes were stored against how many
  /// DeserializeComponent() consumed, and reports a failure through EZ_REPORT_FAILURE if they differ.
  /// That macro is active in every build configuration and goes through the assert handler, which is
  /// replaced here for the duration of the read. Without that the test framework would turn the
  /// report into a debug break and abort the run, so the component types after a broken one would
  /// never be checked.
  static Result RoundTripComponentType(const ezRTTI* pRtti, ezStringBuilder& out_sFailure)
  {
    out_sFailure.Clear();
    g_sReportedFailure.Clear();

    ezDefaultMemoryStreamStorage storage;

    // Write a world that contains a single component of the type under test.
    {
      ezWorldDesc desc("ComponentSerializationTest - Write");
      ezWorld world(desc);
      EZ_LOCK(world.GetWriteMarker());

      ezComponentManagerBase* pManager = world.GetOrCreateManagerForComponentType(pRtti);
      if (pManager == nullptr)
        return Result::Skipped;

      ezGameObject* pObject = nullptr;
      ezGameObjectDesc objectDesc;
      objectDesc.m_bDynamic = true;
      objectDesc.m_sName.Assign("TestObject");
      world.CreateObject(objectDesc, pObject);

      if (pManager->CreateComponent(pObject).IsInvalidated())
        return Result::Skipped;

      ezMemoryStreamWriter writer(&storage);
      ezWorldWriter worldWriter;
      worldWriter.WriteWorld(writer, world);
    }

    // Read it back into a fresh world, watching for the reader's own size check.
    {
      ezWorldDesc desc("ComponentSerializationTest - Read");
      ezWorld world(desc);
      EZ_LOCK(world.GetWriteMarker());

      ezMemoryStreamReader reader(&storage);
      ezWorldReader worldReader;
      if (worldReader.ReadWorldDescription(reader).Failed())
        return Result::Skipped;

      // The reader silently skips types it does not know about, in which case nothing was deserialized.
      if (!worldReader.HasComponentOfType(pRtti))
        return Result::Skipped;

      g_PreviousAssertHandler = ezGetAssertHandler();
      ezSetAssertHandler(SerializationAssertHandler);
      EZ_SCOPE_EXIT(ezSetAssertHandler(g_PreviousAssertHandler));

      worldReader.InstantiateWorld(world);
    }

    if (!g_sReportedFailure.IsEmpty())
    {
      out_sFailure = g_sReportedFailure;
      return Result::SizeMismatch;
    }

    return Result::Ok;
  }
} // namespace ComponentSerializationTestDetail

static ezGameEngineTestComponentSerialization s_GameEngineTestComponentSerialization;

const char* ezGameEngineTestComponentSerialization::GetTestName() const
{
  return "Component Serialization Tests";
}

ezGameEngineTestApplication* ezGameEngineTestComponentSerialization::CreateApplication()
{
  // Uses a project without any plugin configuration: this test only needs a running application with
  // a graphics device, because creating a component also creates its manager and some managers
  // allocate GPU resources.
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "DynamicTextureAtlas");
  return m_pOwnApplication;
}

void ezGameEngineTestComponentSerialization::SetupSubTests()
{
  AddSubTest("Serialize / Deserialize Roundtrip", SubTests::SerializeDeserializeRoundtrip);
}

ezTestAppRun ezGameEngineTestComponentSerialization::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  EZ_IGNORE_UNUSED(iIdentifier);
  EZ_IGNORE_UNUSED(uiInvocationCount);

  ezDynamicArray<const ezRTTI*> componentTypes;

  ezRTTI::ForEachDerivedType<ezComponent>(
    [&](const ezRTTI* pRtti)
    {
      componentTypes.PushBack(pRtti);
    },
    // Components are constructed through their component manager, not through the RTTI allocator
    // (EZ_BEGIN_COMPONENT_TYPE uses ezRTTINoAllocator), so ExcludeNonAllocatable must not be used here.
    ezRTTI::ForEachOptions::ExcludeAbstract);

  // Sort by name so that the output is stable between runs.
  componentTypes.Sort([](const ezRTTI* a, const ezRTTI* b)
    { return a->GetTypeName().Compare(b->GetTypeName()) < 0; });

  EZ_TEST_BOOL_MSG(!componentTypes.IsEmpty(), "No component types found, the test would silently pass.");

  ezUInt32 uiTested = 0;
  ezUInt32 uiSkipped = 0;

  for (const ezRTTI* pRtti : componentTypes)
  {
    ezStringBuilder sFailure;

    const auto res = ComponentSerializationTestDetail::RoundTripComponentType(pRtti, sFailure);

    if (res == ComponentSerializationTestDetail::Result::Skipped)
    {
      ++uiSkipped;
      continue;
    }

    ++uiTested;

    if (res != ComponentSerializationTestDetail::Result::Ok)
    {
      // The reported message already names the type and its version.
      EZ_TEST_FAILURE("Component serialization mismatch", "%s", sFailure.GetData());
    }
  }

  ezLog::Info("Tested {} component types, skipped {}.", uiTested, uiSkipped);

  EZ_TEST_BOOL_MSG(uiTested > 0, "No component type could actually be tested.");

  return ezTestAppRun::Quit;
}
