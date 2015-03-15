using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Web.Script.Serialization;
using System.Xml.Serialization;
using BuildShared;
using Newtonsoft.Json;

namespace BuildMachine
{
  /// <summary>
  /// The actual build process is handled by this class.
  /// A build process consists of the following steps:
  /// SVN update, CMake configuration, Build (compiling) and testing.
  /// </summary>
  public class BuildProcess
  {
    public BuildProcess(string sAbsCMakeWorkspace)
    {
      _CMake = new ezCMake();
      _SVN = new ezSVN();
      _Build = new ezBuild();
      _Test = new ezTest();
      _Result = new BuildProcessResult();
      _Result.Settings.AbsCMakeWorkspace = sAbsCMakeWorkspace;
    }

    /// <summary>
    /// If false is returned, the build machine is misconfigured or another error occurred.
    /// Check the console for more information.
    /// </summary>
    public bool Init()
    {
      if (!LoadSettings())
        return false;

      // CMake needs to be init first as it determines the code dir for SVN.
      if (!_CMake.Init(_Result.Settings))
        return false;

      if (!SaveSettings())
        return false;

      if (!_SVN.Init(_Result.Settings))
        return false;

      if (!_Build.Init(_Result.Settings))
        return false;

      if (!_Test.Init(_Result.Settings))
        return false;

      return true;
    }

    /// <summary>
    /// Runs the build process for the given SVN revision.
    /// </summary>
    /// <param name="iRevision">The SVN revision to which should be updated before building.</param>
    /// <param name="bClean">Whether the builds should be cleaned instead of incremental compiles.</param>
    /// <returns>If false is returned, SVN update failed and the build machine is now in fatal error state
    /// and should not accept any more requests until an admin fixed the issue.</returns>
    public bool Run(int iRevision, bool bClean)
    {
      _Result.Clean();
      _Result.Settings.Revision = iRevision;

      Console.WriteLine("**** Starting building rev {0} of build {1} ****", iRevision, _Result.Settings.Configuration);
      Stopwatch sw = new Stopwatch();
      sw.Start();

      ///
      if (!String.IsNullOrEmpty(_Result.Settings.PreBuildStep))
      {
        ezProcessHelper.RunExternalExe(_Result.Settings.PreBuildStep, "", _Result.Settings.AbsCMakeWorkspace, null);
      }
      ///
      _Result.SVNResult = _SVN.Run(iRevision);
      _Result.CMakeResult = _CMake.Run(_Result.SVNResult);
      _Result.BuildResult = _Build.Run(_Result.CMakeResult, bClean);
      _Result.TestResult = _Test.Run(_Result.CMakeResult, _Result.BuildResult);

      sw.Stop();
      _Result.Duration = sw.Elapsed.TotalSeconds;
      _Result.Success = _Result.SVNResult.Success && _Result.CMakeResult.Success
        && _Result.BuildResult.Success && _Result.TestResult.Success;
      _Result.Settings.Timestamp = LinuxDateTime.Now();
      if (!WriteResultToFile())
      {
        Console.WriteLine("Writing result to file failed!");
      }

      Console.WriteLine("**** Building ended ****");
      // Returns whether the process of building was run properly, not whether everything succeeded or not.
      // If SVN fails somehow we cannot continue and the build machine is set into fatal failure mode.
      return _Result.SVNResult.Success;
    }

    public string GetJSON()
    {
      //JavaScriptSerializer serializer = new JavaScriptSerializer();
      //serializer.MaxJsonLength = 10 * 1024 * 1024;
      try
      {
        string sSerializedResult = JsonConvert.SerializeObject(_Result, Formatting.None /*Formatting.Indented*/,
          new JsonSerializerSettings { ContractResolver = new ezPrivateContractResolver() });
        //string serializedResult = serializer.Serialize(_Result);
        return sSerializedResult;
      }
      catch (Exception ex)
      {
        Console.WriteLine("Serializing json result failed: {0}", ex.Message);
      }
      return null;
    }

    /// <summary>
    /// The filename that the build process results are stored in for the given revision.
    /// </summary>
    /// <param name="iRevision"></param>
    /// <returns></returns>
    public string GetResultFilename(int iRevision)
    {
      string sSafeConfigurationName = _Result.Settings.ConfigurationName.Replace("_", "");
      return string.Format("{0}_{1}.json", sSafeConfigurationName, iRevision);
    }

    #region Properties

    /// <summary>
    /// The filename that the current build process results are stored in.
    /// </summary>
    public string ResultFilename
    {
      get
      {
        return GetResultFilename(_Result.Settings.Revision);
      }
    }

    /// <summary>
    /// The settings of this build machine.
    /// </summary>
    public BuildMachineSettings Settings
    {
      get
      {
        return _Result.Settings;
      }
    }

    #endregion Properties

    #region Private Functions

    bool CheckSettings()
    {
      Type settingsType = _Result.Settings.GetType();
      var properties = settingsType.GetProperties();
      foreach (var property in properties)
      {
        object[] attributes = property.GetCustomAttributes(typeof(ezUserDefinedAttribute), true);
        if (attributes.Count() != 0)
        {
          if (property.PropertyType == typeof(string))
          {
            string value = (string)property.GetValue(_Result.Settings, null);
            if (string.IsNullOrEmpty(value))
            {
              Console.WriteLine("'{0}' needs to be specified in the settings.json, change it and restart this application.", property.Name);
              SaveSettings();
              return false;
            }
          }
        }
      }
      return true;
    }

    bool LoadSettings()
    {
      try
      {
        string sResultPath = System.IO.Path.Combine(_Result.Settings.AbsCMakeWorkspace, "settings.json");
        if (!System.IO.File.Exists(sResultPath))
        {
          Console.WriteLine("The build machine settings are not set yet.\n" + 
            "Open '{0}', change the settings and restart this application.", sResultPath);
          SaveSettings();
          return false;
        }
        string sSerializedResult = System.IO.File.ReadAllText(sResultPath, Encoding.UTF8);
        BuildMachineSettings settings = JsonConvert.DeserializeObject<BuildMachineSettings>(sSerializedResult);

        // We take the whole settings except for the 'AbsCMakeWorkspace' which is the only parameter that is given to as as
        // a command line parameter and could differ if someone copied the settings from another build machine.
        settings.AbsCMakeWorkspace = _Result.Settings.AbsCMakeWorkspace;
        if (String.IsNullOrEmpty(settings.BuildType))
          settings.BuildType = "RelWithDebInfo";
        _Result.Settings = settings;

        if (!CheckSettings())
          return false;
      }
      catch (Exception ex)
      {
        Console.WriteLine("Writing json file failed: {0}", ex.Message);
        return false;
      }
      return true;
    }

    bool SaveSettings()
    {
      try
      {
        string serializedResult = JsonConvert.SerializeObject(_Result.Settings, Formatting.Indented);
        string sResultPath = System.IO.Path.Combine(_Result.Settings.AbsCMakeWorkspace, "settings.json");
        System.IO.File.WriteAllText(sResultPath, serializedResult, Encoding.UTF8);
      }
      catch (Exception ex)
      {
        Console.WriteLine("Writing json file failed: {0}", ex.Message);
        return false;
      }
      return true;
    }

    bool WriteResultToFile()
    {
      string sResult = GetJSON();
      if (string.IsNullOrEmpty(sResult))
      {
        return false;
      }

      try
      {
        string sResultPath = System.IO.Path.Combine(_Result.Settings.AbsOutputFolder, ResultFilename);
        System.IO.File.WriteAllText(sResultPath, sResult, Encoding.UTF8);
      }
      catch (Exception ex)
      {
        Console.WriteLine("Writing json result failed: {0}", ex.Message);
        return false;
      }
      return true;
    }

    #endregion Private Functions

    public class BuildProcessResult : BuildStepResults
    {
      public BuildProcessResult()
      {
        Settings = new BuildMachineSettings();
      }

      public ezSVN.SVNResult SVNResult { get; set; }
      public ezCMake.CMakeResult CMakeResult { get; set; }
      public ezBuild.BuildResult BuildResult { get; set; }
      public ezTest.TestResult TestResult { get; set; }
      public BuildMachineSettings Settings { get; set; }

      public override void Clean()
      {
        base.Clean();
        SVNResult = null;
        CMakeResult = null;
        BuildResult = null;
        TestResult = null;
      }
    }

    #region Private Members

    ezCMake _CMake;
    ezSVN _SVN;
    ezBuild _Build;
    ezTest _Test;
    BuildProcessResult _Result;

    #endregion Private Members
  }
}
