#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/Progress.h>

EZ_CREATE_SIMPLE_TEST(Utility, Progress)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple progress")
  {
    ezProgress progress;
    {
      ezProgressRange progressRange = ezProgressRange("TestProgress", 4, false, &progress);

      EZ_TEST_FLOAT(progress.GetCompletion(), 0.0f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(progress.GetMainDisplayText() == "TestProgress");

      progressRange.BeginNextStep("Step1");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.0f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(progress.GetStepDisplayText() == "Step1");

      progressRange.BeginNextStep("Step2");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.25f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(progress.GetStepDisplayText() == "Step2");

      progressRange.BeginNextStep("Step3");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.5f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(progress.GetStepDisplayText() == "Step3");

      progressRange.BeginNextStep("Step4");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.75f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(progress.GetStepDisplayText() == "Step4");
    }

    EZ_TEST_FLOAT(progress.GetCompletion(), 1.0f, ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Weighted progress")
  {
    ezProgress progress;
    {
      ezProgressRange progressRange = ezProgressRange("TestProgress", 4, false, &progress);
      progressRange.SetStepWeighting(2, 2.0f);

      EZ_TEST_FLOAT(progress.GetCompletion(), 0.0f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0+1", 2);
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.2f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2"); // this step should have twice the weight as the other steps.
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.4f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step3");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.8f, ezMath::DefaultEpsilon<float>());
    }

    EZ_TEST_FLOAT(progress.GetCompletion(), 1.0f, ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Nested progress")
  {
    ezProgress progress;
    {
      ezProgressRange progressRange = ezProgressRange("TestProgress", 4, false, &progress);
      progressRange.SetStepWeighting(2, 2.0f);

      EZ_TEST_FLOAT(progress.GetCompletion(), 0.0f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.0f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step1");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.2f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.4f, ezMath::DefaultEpsilon<float>());

      {
        ezProgressRange nestedRange = ezProgressRange("Nested", 5, false, &progress);
        nestedRange.SetStepWeighting(1, 4.0f);

        EZ_TEST_FLOAT(progress.GetCompletion(), 0.4f, ezMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep0");
        EZ_TEST_FLOAT(progress.GetCompletion(), 0.4f, ezMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep1");
        EZ_TEST_FLOAT(progress.GetCompletion(), 0.45f, ezMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep2");
        EZ_TEST_FLOAT(progress.GetCompletion(), 0.65f, ezMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep3");
        EZ_TEST_FLOAT(progress.GetCompletion(), 0.7f, ezMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep4");
        EZ_TEST_FLOAT(progress.GetCompletion(), 0.75f, ezMath::DefaultEpsilon<float>());
      }
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.8f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step3");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.8f, ezMath::DefaultEpsilon<float>());
    }

    EZ_TEST_FLOAT(progress.GetCompletion(), 1.0f, ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Nested progress with manual completion")
  {
    ezProgress progress;
    {
      ezProgressRange progressRange = ezProgressRange("TestProgress", 3, false, &progress);
      progressRange.SetStepWeighting(1, 2.0f);

      EZ_TEST_FLOAT(progress.GetCompletion(), 0.0f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.0f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step1");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.25f, ezMath::DefaultEpsilon<float>());

      {
        ezProgressRange nestedRange = ezProgressRange("Nested", false, &progress);

        EZ_TEST_FLOAT(progress.GetCompletion(), 0.25f, ezMath::DefaultEpsilon<float>());

        nestedRange.SetCompletion(0.5);
        EZ_TEST_FLOAT(progress.GetCompletion(), 0.5f, ezMath::DefaultEpsilon<float>());
      }
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.75f, ezMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2");
      EZ_TEST_FLOAT(progress.GetCompletion(), 0.75f, ezMath::DefaultEpsilon<float>());
    }

    EZ_TEST_FLOAT(progress.GetCompletion(), 1.0f, ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Progress Events")
  {
    ezUInt32 uiNumProgressUpdatedEvents = 0;

    ezProgress progress;
    progress.m_Events.AddEventHandler([&](const ezProgressEvent& e)
      {
      if (e.m_Type == ezProgressEvent::Type::ProgressChanged)
      {
        ++uiNumProgressUpdatedEvents;
        EZ_TEST_FLOAT(e.m_pProgressbar->GetCompletion(), uiNumProgressUpdatedEvents * 0.25f, ezMath::DefaultEpsilon<float>());
      } });

    {
      ezProgressRange progressRange = ezProgressRange("TestProgress", 4, false, &progress);

      progressRange.BeginNextStep("Step1");
      progressRange.BeginNextStep("Step2");
      progressRange.BeginNextStep("Step3");
      progressRange.BeginNextStep("Step4");
    }

    EZ_TEST_FLOAT(progress.GetCompletion(), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_INT(uiNumProgressUpdatedEvents, 4);
  }
}
