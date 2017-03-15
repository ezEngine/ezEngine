using BuildShared;
using System;
using System.Text;

namespace BuildMachine
{
  class ezTestDefault : ezTestTemplate
  {
    public override ezTest.TestTargetResult BuildTarget(ezCMake.TestTarget target, BuildShared.BuildMachineSettings settings)
    {
      ezTest.TestTargetResult res = new ezTest.TestTargetResult();
      res.Name = target.Name;
      res.NeedsHardwareAccess = target.NeedsHardwareAccess;
      res.Experimental = target.Experimental;

      string absBinDir = System.IO.Path.Combine(settings.AbsBinPath, settings.Configuration);
      string absBinFilename = System.IO.Path.Combine(absBinDir, target.Name);
      string outputFilename = GetOutputFileName(target, settings);
      string absOutputPath = System.IO.Path.Combine(settings.AbsOutputFolder, outputFilename);
      if (!DeleteOutputFile(absOutputPath, ref res))
        return res;

      res.ProcessRes = ezProcessHelper.RunExternalExe(absBinFilename, GetDefaultTestArgs(absOutputPath, settings), absBinDir, res);
      res.Success = (res.ProcessRes.ExitCode == 0);
      res.Duration = res.ProcessRes.Duration;

      if (System.IO.File.Exists(absOutputPath))
      {
        res.TestResultJSON = System.IO.File.ReadAllText(absOutputPath, Encoding.UTF8);
      }
      else
      {
        res.Error("No output file present!");
      }

      if (!res.Success && !res.Experimental)
        res.Error("Testing '{0}' failed!", res.Name);
      return res;
    }
  }
}
