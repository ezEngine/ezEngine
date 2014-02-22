using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Web.Script.Serialization;
using System.Xml.Serialization;
using Newtonsoft.Json;

namespace BuildShared
{
  public static class LinuxDateTime
  {
    /// <summary>
    /// Returns the current time as seconds since unix epoch.
    /// </summary>
    public static long Now()
    {
      var timeSpan = (DateTime.UtcNow - new DateTime(1970, 1, 1, 0, 0, 0));
      return (long)timeSpan.TotalSeconds;
    }
  }

  /// <summary>
  /// Hold the settings of a build machine instance. Stored in 'settings.json'
  /// in the CMake build folder.
  /// </summary>
  public class BuildMachineSettings
  {
    public BuildMachineSettings()
    {
    }

    public string AbsCMakeWorkspace { get; set; }
    public string AbsCodePath { get; set; }
    public string AbsBinPath { get; set; }
    public string AbsOutputFolder { get; set; }
    public string Configuration { get; set; }
    public string ConfigurationName { get; set; }
    public bool DirectHardwareAccess { get; set; }
    public string ServerAddress { get; set; }

    [JsonIgnoreAttribute]
    public int Revision { get; set; }
  }

  public class BuildStepResults
  {
    public BuildStepResults()
    {
      Clean();
    }

    public virtual void Clean()
    {
      Success = false;
      Errors = "";
      Duration = 0.0;
    }

    public void Error(string format, params object[] arg)
    {
      string sError = String.Format(format, arg);
      Console.WriteLine(sError);
      Errors = Errors + ("\n" + sError);
    }

    public bool Success { get; set; }
    public double Duration { get; set; }
    public string Errors { get; set; }

  }
}
