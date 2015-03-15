using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using BuildShared;

namespace BuildMachine
{
  /// <summary>
  /// Build template implementation for XCode projects.
  /// </summary>
  public class BuildOsxXcode : ezBuildTemplate
  {
    public BuildOsxXcode(BuildMachineSettings settings)
    {
      _settings = settings;
    }

    public ezBuild.BuildTargetResult BuildTarget(ezCMake.BuildTarget target, string sAbsWorkingDir)
    {
      ezBuild.BuildTargetResult res = new ezBuild.BuildTargetResult();
      res.Experimental = target.Experimental;
      res.Name = target.Name;

      res.ProcessRes = ezProcessHelper.RunExternalExe("xcodebuild", string.Format("-configuration {0} -target {1}", _settings.BuildType, target.Name), sAbsWorkingDir, res);
      res.Success = (res.ProcessRes.ExitCode == 0);
      CleanUpXcodeOutput(res.ProcessRes);
      return res;
    }

    public bool CleanSolution(string sAbsWorkingDir)
    {
      ezProcessHelper.ProcessResult res = ezProcessHelper.RunExternalExe("xcodebuild", "-alltargets clean", sAbsWorkingDir, null);
      return res.ExitCode == 0;
    }

    private void CleanUpXcodeOutput(ezProcessHelper.ProcessResult processRes)
    {
      if (processRes == null || String.IsNullOrEmpty(processRes.StdOut))
        return;

      List<string> newLines = new List<string>();
      string[] lines = processRes.StdOut.Split('\n');
      int iLineCount = lines.Count();
      for (int i = 0; i < iLineCount; )
      {
        // Skip the mega verbose output of CompileC.
        if ((i + 3 < iLineCount) &&
          lines[i + 0].StartsWith("CompileC") &&
          lines[i + 1].StartsWith("    cd") &&
          lines[i + 2].StartsWith("    ") &&
          lines[i + 3].StartsWith("    /"))
        {
          newLines.Add(lines[i]);
          i = i + 4;
        }
        else
        {
          newLines.Add(lines[i]);
          i++;
        }
      }

      string newStdOut = string.Join("\n", newLines);
      processRes.StdOut = newStdOut;
    }

    #region Private Members

    BuildMachineSettings _settings;

    #endregion Private Members
  }
}
