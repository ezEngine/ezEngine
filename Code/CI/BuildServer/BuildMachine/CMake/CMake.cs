using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Web.Script.Serialization;
using BuildShared;

namespace BuildMachine
{
  /// <summary>
  /// This class takes care of the CMake configuration in a build process.
  /// </summary>
  public class ezCMake
  {
    public ezCMake()
    {
    }

    public bool Init(BuildMachineSettings settings)
    {
      _Result.Clean();
      _Settings = settings;

      try
      {
        // Make sure the given parameter is valid.
        if (!System.IO.Path.IsPathRooted(_Settings.AbsCMakeWorkspace))
        {
          Console.WriteLine("CMake Init failed: The path '{0}' is not absolute!", _Settings.AbsCMakeWorkspace);
          return false;
        }

        if (!System.IO.Directory.Exists(_Settings.AbsCMakeWorkspace))
        {
          Console.WriteLine("CMake Init failed: The path '{0}' does not exist!", _Settings.AbsCMakeWorkspace);
          return false;
        }

        Console.WriteLine("CMake Init: Workspace: '{0}'.", _Settings.AbsCMakeWorkspace);

        // Run process to get configuration and code dir.
        ezProcessHelper.ProcessResult res = ezProcessHelper.RunExternalExe("cmake", ".", _Settings.AbsCMakeWorkspace, _Result);

        // Determine the configuration this server is taking care of if non is set.
        if (String.IsNullOrEmpty(_Settings.Configuration) && !DetermineConfiguration())
          return false;

        Console.WriteLine("CMake Init: Configuration: '{0}'.", _Settings.Configuration);

        // Determine the code directory this CMake workspace belongs to.
        if (!DetermineCodeDirectory())
          return false;

        // Determine the bin directory where executables are build to.
        if (!DetermineBinDirectory())
          return false;

        Console.WriteLine("CMake Init: Code Path: '{0}'.", _Settings.AbsCodePath);
      }
      catch (Exception ex)
      {
        Console.WriteLine(ex);
        return false;
      }

      return true;
    }

    public CMakeResult Run(ezSVN.SVNResult svnResults)
    {
      _Result.Clean();

      Console.WriteLine("*** Starting CMake of '{0}' ***", _Settings.AbsCMakeWorkspace);

      if (!svnResults.Success)
      {
        _Result.Error("CMake failed: SVN step failed, aborting!");
        return _Result;
      }

      try
      {
        // Run CMake
        _Result.ProcessRes = ezProcessHelper.RunExternalExe("cmake", ".", _Settings.AbsCMakeWorkspace, _Result);
        _Result.Duration = _Result.ProcessRes.Duration;

        if (_Result.ProcessRes.ExitCode != 0)
        {
          _Result.Error("CMake failed: CMake returned ErrorCode: {0}!", _Result.ProcessRes.ExitCode);
          return _Result;
        }

        // Gather build targets
        if (!DetermineBuildTargets())
          return _Result;

        // Gather test targets
        if (!DetermineTestTargets())
          return _Result;
      }
      catch (Exception ex)
      {
        _Result.Error(ex.Message);
        return _Result;
      }

      _Result.Success = true;
      return _Result;
    }

    #region Private Functions

    bool DetermineConfiguration()
    {
      string sConfigurationFile = System.IO.Path.Combine(_Settings.AbsCMakeWorkspace, "Configuration.txt");
      if (!System.IO.File.Exists(sConfigurationFile))
      {
        Console.WriteLine("CMake Init warning: '{0}' does not exist!", sConfigurationFile);
      }
      else
      {
        string[] lines = System.IO.File.ReadAllLines(sConfigurationFile, Encoding.UTF8);
        if (lines.Length == 0 || string.IsNullOrEmpty(lines[0]))
        {
          Console.WriteLine("CMake Init warning: '{0}' is empty!", sConfigurationFile);
        }
        else
          _Settings.Configuration = lines[0];
      }

      if (String.IsNullOrEmpty(_Settings.Configuration))
      {
        Console.WriteLine("CMake Init failed: Configuration not cached and couldn't be determined");
      }

      return !String.IsNullOrEmpty(_Settings.Configuration);
    }

    bool DetermineCodeDirectory()
    {
      const string sCodeDirToken = "CMAKE_HOME_DIRECTORY:INTERNAL=";

      string sOutput = _Settings.AbsCodePath;
      bool bRes = DetermineCacheVariable(sCodeDirToken, ref sOutput);
      _Settings.AbsCodePath = sOutput;
      return bRes;
    }

    bool DetermineBinDirectory()
    {
      const string sBinDirToken = "EZ_OUTPUT_DIRECTORY_DLL:PATH=";

      string sOutput = _Settings.AbsBinPath;
      bool bRes = DetermineCacheVariable(sBinDirToken, ref sOutput);
      _Settings.AbsBinPath = sOutput;
      return bRes;
    }

    bool DetermineCacheVariable(string sCacheToken, ref string out_sResult)
    {
      string sCacheFile = System.IO.Path.Combine(_Settings.AbsCMakeWorkspace, "CMakeCache.txt");
      if (!System.IO.File.Exists(sCacheFile))
      {
        Console.WriteLine("CMake Init warning: '{0}' does not exist!", sCacheFile);
      }
      else
      {
        string[] lines = System.IO.File.ReadAllLines(sCacheFile, Encoding.UTF8);
        foreach (string line in lines)
        {
          if (line.StartsWith(sCacheToken))
          {
            string sValue = line.Substring(sCacheToken.Length);
            if (!String.IsNullOrEmpty(sValue))
            {
              out_sResult = sValue;
              break;
            }
          }
        }
      }

      if (string.IsNullOrEmpty(out_sResult))
      {
        Console.WriteLine("CMake Init failed: '{0}' does not contain '{1}' and it is not cached!", sCacheFile, sCacheToken);
        return false;
      }
      return true;
    }

    bool DetermineBuildTargets()
    {
      List<BuildTarget> targets = new List<BuildTarget>();
      HashSet<string> targetNames = new HashSet<string>();

      // Load Targets file
      string sTargetsFile = System.IO.Path.Combine(_Settings.AbsCMakeWorkspace, "Targets.txt");
      if (!System.IO.File.Exists(sTargetsFile))
      {
        _Result.Error("CMake Run failed: '{0}' does not exist!", sTargetsFile);
        return false;
      }
      else
      {
        string[] lines = System.IO.File.ReadAllLines(sTargetsFile, Encoding.UTF8);
        if (lines.Length == 0)
        {
          _Result.Error("CMake Run failed: '{0}' is empty!", sTargetsFile);
          return false;
        }

        foreach (string line in lines)
        {
          string[] parts = line.Split('|');
          if (parts.Length != 5)
          {
            _Result.Error("CMake Run warning: Target line is invalid: '{0}'!", line);
            continue;
          }

          if (targetNames.Contains(parts[0]))
          {
            _Result.Error("CMake Run warning: Target line is invalid: '{0}' Target already exists!", line);
            continue;
          }

          BuildTarget target = new BuildTarget();
          target.Name = parts[0];
          target.RelativePath = parts[1];
          target.Experimental = parts[2] == "1";
          target.BuildType = parts[3];
          target.Dependencies = parts[4].Split(';').ToList();
          targetNames.Add(parts[0]);
          targets.Add(target);
        }
      }

      // Clean dependencies (we are only interested in internal dependencies)
      foreach (BuildTarget target in targets)
      {
        List<string> cleanedDependencies = new List<string>();
        foreach (string dep in target.Dependencies)
        {
          if (targetNames.Contains(dep))
          {
            cleanedDependencies.Add(dep);
          }
        }
        target.Dependencies = cleanedDependencies;
      }

      // Sort targets
      _Result.BuildTargets = new List<BuildTarget>();

      List<string> availableDependencies = new List<string>();
      int iLastCount = targets.Count;
      while (targets.Count > 0)
      {
        foreach (BuildTarget target in targets)
        {
          bool bMeetDependencies = !target.Dependencies.Except(availableDependencies).Any();
          if (bMeetDependencies)
          {
            _Result.BuildTargets.Add(target);
            availableDependencies.Add(target.Name);
            targets.Remove(target);
            break;
          }
        }

        if (iLastCount == targets.Count)
        {
          foreach (BuildTarget target in targets)
          {
            _Result.Error("CMake Run error: Target's dependencies can not be resolved: '{0}' ({1}) Dropping target!", target.Name, target.Dependencies.ToString());
          }
          break;
        }
        iLastCount = targets.Count;
      }

      return true;
    }

    bool DetermineTestTargets()
    {
      List<TestTarget> targets = new List<TestTarget>();
      HashSet<string> targetNames = new HashSet<string>();

      // Load Tests file
      string sTestsFile = System.IO.Path.Combine(_Settings.AbsCMakeWorkspace, "Tests.txt");
      if (!System.IO.File.Exists(sTestsFile))
      {
        _Result.Error("CMake Run failed: '{0}' does not exist!", sTestsFile);
        return false;
      }
      else
      {
        string[] lines = System.IO.File.ReadAllLines(sTestsFile, Encoding.UTF8);

        foreach (string line in lines)
        {
          string[] parts = line.Split('|');
          if (parts.Length != 3)
          {
            _Result.Error("CMake Run warning: Test line is invalid: '{0}'!", line);
            continue;
          }

          if (targetNames.Contains(parts[0]))
          {
            _Result.Error("CMake Run warning: Test line is invalid: '{0}' Test target already exists!", line);
            continue;
          }

          TestTarget target = new TestTarget();
          target.Name = parts[0];
          target.NeedsHardwareAccess = parts[1] == "1";
          target.Experimental = parts[2] == "1";
          targetNames.Add(parts[0]);
          targets.Add(target);
        }
      }
      _Result.TestTargets = targets;
      return true;
    }

    #endregion Private Functions

    /// <summary>
    /// The results of the CMake configuration step in the build process.
    /// </summary>
    public class CMakeResult : BuildStepResults
    {
      public ezProcessHelper.ProcessResult ProcessRes { get; set; }
      [ScriptIgnore]
      public List<BuildTarget> BuildTargets { get; set; }
      [ScriptIgnore]
      public List<TestTarget> TestTargets { get; set; }

      public override void Clean()
      {
        base.Clean();
        ProcessRes = null;
        BuildTargets = null;
      }
    }

    /// <summary>
    /// Defines a build target, used by the ezBuild class.
    /// </summary>
    public class BuildTarget
    {
      public string Name { get; set; }
      public string BuildType { get; set; }
      public bool Experimental { get; set; }
      public string RelativePath { get; set; }
      public List<string> Dependencies { get; set; }
    }

    /// <summary>
    /// Defines a test target, used by the ezTest class.
    /// </summary>
    public class TestTarget
    {
      public string Name { get; set; }
      public bool NeedsHardwareAccess { get; set; }
      public bool Experimental { get; set; }
    }

    #region Private Members

    CMakeResult _Result = new CMakeResult();
    BuildMachineSettings _Settings = null;

    #endregion Private Members
  }
}
