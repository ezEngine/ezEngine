using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using BuildShared;

namespace BuildMachine
{
  /// <summary>
  /// This class takes care of the testing in a build process.
  /// </summary>
  public class ezTest
  {
    public ezTest()
    {
    }

    public bool Init(BuildMachineSettings settings)
    {
      _Settings = settings;
      return true;
    }

    public TestResult Run(ezCMake.CMakeResult cmakeResults, ezBuild.BuildResult buildResults)
    {
      _Result.Clean();
      Console.WriteLine("*** Starting Test ***");

      if (!cmakeResults.Success)
      {
        _Result.Error("Test failed: CMake step failed, aborting!");
        return _Result;
      }

      Stopwatch sw = new Stopwatch();
      sw.Start();

      int iFailedTargets = 0;
      List<ezCMake.TestTarget> targets = cmakeResults.TestTargets;
      foreach (ezCMake.TestTarget target in targets)
      {
        Console.WriteLine("** Starting Test of target '{0}' **", target.Name);
        // Only test targets which compiled successfully.
        ezBuild.BuildTargetResult buildTargetResult = buildResults.BuildTargetResults.Find(item => item.Name == target.Name);

        TestTargetResult testResult = null;

        if (buildTargetResult == null || !buildTargetResult.Success)
        {
          // Dummy result for non-testable targets.
          testResult = new TestTargetResult();
          testResult.Name = target.Name;
          testResult.NeedsHardwareAccess = target.NeedsHardwareAccess;
          testResult.Experimental = target.Experimental;
          testResult.ProcessRes = null;

          if (buildTargetResult == null)
            testResult.Error("Target '{0}' has no equivalent build target, test aborted!", testResult.Name);
          else if (!buildTargetResult.Success)
            testResult.Error("Target '{0}' was not build successfully, test aborted!", testResult.Name);
        }
        else
        {
          testResult = RunTest(target);
        }

        if (!testResult.Success && !testResult.Experimental)
          iFailedTargets++;

        _Result.TestTargetResults.Add(testResult);
      }

      sw.Stop();
      _Result.Duration = sw.Elapsed.TotalSeconds;
      _Result.Success = iFailedTargets == 0;

      if (iFailedTargets > 0)
        _Result.Error("Test failed: {0} targets failed the test!", iFailedTargets);
      return _Result;
    }

    #region Private Functions

    ezTest.TestTargetResult RunTest(ezCMake.TestTarget target)
    {
      ezTest.TestTargetResult res = new ezTest.TestTargetResult();
      res.Name = target.Name;
      res.NeedsHardwareAccess = target.NeedsHardwareAccess;
      res.Experimental = target.Experimental;

      string sAbsBinDir = System.IO.Path.Combine(_Settings.AbsBinPath, _Settings.Configuration);
      string sAbsBinFilename = System.IO.Path.Combine(sAbsBinDir, target.Name);
      string sAbsOutputPath = System.IO.Path.Combine(_Settings.AbsOutputFolder, string.Format("{0}_{1}_{2}.json", _Settings.Configuration, _Settings.Revision, target.Name));

      // In case we are re-running a build process the output file may already exist and we need to make sure it doesn't
      // contaminate the new test run.
      if (System.IO.File.Exists(sAbsOutputPath))
      {
        try
        {
          System.IO.File.Delete(sAbsOutputPath);
        }
        catch (Exception ex)
        {
          res.Error("Error deleting test output file ({0}): {1}", sAbsOutputPath, ex.Message);
          return res;
        }
      }

      res.ProcessRes = ezProcessHelper.RunExternalExe(sAbsBinFilename, string.Format("-json \"{0}\" -rev {1} -nogui -all", sAbsOutputPath, _Settings.Revision), sAbsBinDir, res);
      res.Success = (res.ProcessRes.ExitCode == 0);
      res.Duration = res.ProcessRes.Duration;

      if (System.IO.File.Exists(sAbsOutputPath))
      {
        res.TestResultJSON = System.IO.File.ReadAllText(sAbsOutputPath, Encoding.UTF8);
      }
      else
      {
        res.Error("No output file present!");
      }

      if (!res.Success && !res.Experimental)
        res.Error("Testing '{0}' failed!", res.Name);
      return res;
    }

    #endregion Private Functions

    /// <summary>
    /// The results of the test step in the build process.
    /// </summary>
    public class TestResult : BuildStepResults
    {
      public List<TestTargetResult> TestTargetResults { get; set; }

      public override void Clean()
      {
        base.Clean();
        TestTargetResults = new List<TestTargetResult>();
      }
    }

    /// <summary>
    /// The results for a single test application run.
    /// </summary>
    public class TestTargetResult : BuildStepResults
    {
      public string Name { get; set; }
      public bool NeedsHardwareAccess { get; set; }
      public bool Experimental { get; set; }
      public string TestResultJSON { get; set; }
      public ezProcessHelper.ProcessResult ProcessRes { get; set; }
    }

    #region Private Members

    TestResult _Result = new TestResult();
    BuildMachineSettings _Settings = null;


    #endregion Private Members
  }
}
