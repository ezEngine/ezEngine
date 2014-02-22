using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using BuildShared;

namespace BuildMachine
{
  /// <summary>
  /// Build template implementation for make files.
  /// </summary>
  public class BuildMake : ezBuildTemplate
  {
    public BuildMake()
    {
    }

    public ezBuild.BuildTargetResult BuildTarget(ezCMake.BuildTarget target, string sAbsWorkingDir)
    {
      ezBuild.BuildTargetResult res = new ezBuild.BuildTargetResult();
      res.Experimental = target.Experimental;
      res.Name = target.Name;

      res.ProcessRes = ezProcessHelper.RunExternalExe("make", target.Name, sAbsWorkingDir, res);
      res.Success = (res.ProcessRes.ExitCode == 0);
      return res;
    }
  }
}
