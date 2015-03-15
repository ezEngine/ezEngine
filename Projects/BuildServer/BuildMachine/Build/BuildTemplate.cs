using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using BuildShared;

namespace BuildMachine
{
  /// <summary>
  /// Static class that contains the build template factory.
  /// </summary>
  public static class ezBuildTemplateFactory
  {
    /// <summary>
    /// Returns the build template for the given configuration.
    /// </summary>
    /// <param name="sConfiguration">Configuration as returned by CMake.</param>
    /// <returns></returns>
    public static ezBuildTemplate Create(BuildMachineSettings settings, string sBuildType)
    {
      if (sBuildType == "C++")
      {
        if (settings.Configuration.StartsWith("WinVs"))
        {
          bool bIs64Bit = settings.Configuration.Contains("64");
          if (settings.Configuration.Contains("2012"))
          {
            return new ezBuildWinVS(settings, VSVersion.VS2012, bIs64Bit);
          }
          else if (settings.Configuration.Contains("2013"))
          {
            return new ezBuildWinVS(settings, VSVersion.VS2013, bIs64Bit);
          }
        }
        else if (settings.Configuration.StartsWith("Osx"))
        {
          if (settings.Configuration.Contains("MakeClang"))
          {
            return new BuildMake(settings);
          }
          else if (settings.Configuration.Contains("XcodeClang"))
          {
            return new BuildOsxXcode(settings);
          }
        }
        else if (settings.Configuration.StartsWith("Linux"))
        {
          if (settings.Configuration.Contains("MakeGcc"))
          {
            return new BuildMake(settings);
          }
        }
      }
      else if (sBuildType == "D")
      {
        if (settings.Configuration.StartsWith("WinVs"))
        {
          bool bIs64Bit = settings.Configuration.Contains("64");
          if (settings.Configuration.Contains("2012"))
          {
            return new ezBuildWinD(settings, VSVersion.VS2012, bIs64Bit);
          }
          else if (settings.Configuration.Contains("2013"))
          {
            return new ezBuildWinD(settings, VSVersion.VS2013, bIs64Bit);
          }
        }
      }
      return null;
    }
  }

  /// <summary>
  /// The build template interface that abstracts the build process.
  /// </summary>
  public interface ezBuildTemplate
  {
    ezBuild.BuildTargetResult BuildTarget(ezCMake.BuildTarget target, string sAbsWorkingDir);
    bool CleanSolution(string sAbsWorkingDir);
  }
}
