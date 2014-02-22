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
    public static ezBuildTemplate Create(string sConfiguration)
    {
      if (sConfiguration.StartsWith("WinVs"))
      {
        bool bIs64Bit = sConfiguration.Contains("64");
        if (sConfiguration.Contains("2010"))
        {
          return new ezBuildWinVS(VSVersion.VS2010, bIs64Bit);
        }
        else if (sConfiguration.Contains("2012"))
        {
          return new ezBuildWinVS(VSVersion.VS2012, bIs64Bit);
        }
        else if (sConfiguration.Contains("2013"))
        {
          return new ezBuildWinVS(VSVersion.VS2013, bIs64Bit);
        }
      }
      else if (sConfiguration.StartsWith("Osx"))
      {
        if (sConfiguration.Contains("MakeClang"))
        {
          return new BuildMake();
        }
        else if (sConfiguration.Contains("XcodeClang"))
        {
          return new BuildOsxXcode();
        }
      }
      else if (sConfiguration.StartsWith("Linux"))
      {
        if (sConfiguration.Contains("MakeGcc"))
        {
          return new BuildMake();
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
  }
}
