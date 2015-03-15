using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Web.Script.Serialization;
using System.Xml.Serialization;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;

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

    /// <summary>
    /// Returns the passed DateTime as seconds since unix epoch.
    /// </summary>
    public static long DateTimeToTimestamp(DateTime dateTime)
    {
      var timeSpan = (dateTime - new DateTime(1970, 1, 1, 0, 0, 0));
      return (long)timeSpan.TotalSeconds;
    }
  }


  [System.AttributeUsage(System.AttributeTargets.Property)]
  public class ezPrivateAttribute : System.Attribute
  {
  }

  [System.AttributeUsage(System.AttributeTargets.Property)]
  public class ezUserDefinedAttribute : System.Attribute
  {
  }

  /// <summary>
  /// Using this ContractResolver, all properties marked with ezPrivateAttribute are omitted by the json serializer.
  /// </summary>
  public class ezPrivateContractResolver : DefaultContractResolver
  {
    protected override JsonProperty CreateProperty(MemberInfo member, MemberSerialization memberSerialization)
    {
      var attributes = member.GetCustomAttributes(typeof(ezPrivateAttribute), true);
      JsonProperty property = base.CreateProperty(member, memberSerialization);
      if (attributes.Count() != 0)
      {
        property.ShouldSerialize =
          instance =>
          {
            return false;
          };
      }

      return property;
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

    [ezPrivateAttribute, ezUserDefinedAttribute]
    public string AbsCMakeWorkspace { get; set; }
    [ezPrivateAttribute]
    public string AbsCodePath { get; set; }
    [ezPrivateAttribute]
    public string AbsBinPath { get; set; }
    [ezPrivateAttribute, ezUserDefinedAttribute]
    public string AbsOutputFolder { get; set; }
    [ezPrivateAttribute, ezUserDefinedAttribute]
    public string SVNUsername { get; set; }
    [ezPrivateAttribute, ezUserDefinedAttribute]
    public string SVNPassword { get; set; }
    [ezPrivateAttribute, ezUserDefinedAttribute]
    public string ServerAddress { get; set; }
    [ezPrivateAttribute]
    public string PreBuildStep { get; set; }

    public string Configuration { get; set; }
    public string BuildType { get; set; }
    [ezUserDefinedAttribute]
    public string ConfigurationName { get; set; }
    public bool DirectHardwareAccess { get; set; }
    public int Revision { get; set; }
    public long Timestamp { get; set; }
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
