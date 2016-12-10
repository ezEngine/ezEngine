using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using BuildShared;

namespace BuildMachine
{
  public enum VSVersion
  {
    VS2012,
    VS2013,
    VS2015
  };

  /// <summary>
  /// Build template implementation for the Visual Studio compiler.
  /// </summary>
  public class ezBuildWinVS : ezBuildTemplate
  {
    public ezBuildWinVS(BuildMachineSettings settings, VSVersion version, bool bIs64Bit)
    {
      _settings = settings;
      _Version = version;
      _bIs64Bit = bIs64Bit;
    }

    public ezBuild.BuildTargetResult BuildTarget(ezCMake.BuildTarget target, string sAbsWorkingDir)
    {
      ezBuild.BuildTargetResult res = new ezBuild.BuildTargetResult();
      res.Experimental = target.Experimental;
      res.Name = target.Name;

      if (!WriteBatFile(target, sAbsWorkingDir))
      {
        res.Error("Build '{0}' failed: bat file could not be written ({1})!", target.Name, System.IO.Path.Combine(sAbsWorkingDir, "build.bat"));
        return res;
      }

      CleanupErrorsAndWarnings(sAbsWorkingDir);
      res.ProcessRes = ezProcessHelper.RunExternalExe("cmd", "/c build.bat", sAbsWorkingDir, res);
      ReadErrorsAndWarnings(res, sAbsWorkingDir);
      res.Success = (res.ProcessRes.ExitCode == 0);
      return res;
    }

    public bool CleanSolution(string sAbsWorkingDir)
    {
      if (!WriteBatFileCleanSolution(sAbsWorkingDir))
        return false;

      ezProcessHelper.ProcessResult res = ezProcessHelper.RunExternalExe("cmd", "/c build.bat", sAbsWorkingDir, null);
      return res.ExitCode == 0;
    }

    #region Private Functions

    string GetVisualStudioCommandlineInit()
    {
      switch (_Version)
      {
        case VSVersion.VS2012:
          if (_bIs64Bit)
            return "call \"C:\\Program Files (x86)\\Microsoft Visual Studio 11.0\\VC\\vcvarsall.bat\" amd64\r\n";
          else
            return "call \"C:\\Program Files (x86)\\Microsoft Visual Studio 11.0\\VC\\vcvarsall.bat\" x86\r\n";
        case VSVersion.VS2013:
          if (_bIs64Bit)
            return "call \"C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\vcvarsall.bat\" amd64\r\n";
          else
            return "call \"C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\vcvarsall.bat\" x86\r\n";
        case VSVersion.VS2015:
          if (_bIs64Bit)
            return "call \"C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat\" amd64\r\n";
          else
            return "call \"C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat\" x86\r\n";
        default:
          return "";
      }
    }

    bool WriteBatFileCleanSolution(string sAbsWorkingDir)
    {
      try
      {
        string sBatFileContent = GetVisualStudioCommandlineInit();
        sBatFileContent += string.Format("msbuild \"ezEngine.sln\" /p:Configuration={0} /t:clean", _settings.BuildType);

        string sAbsFilePath = System.IO.Path.Combine(sAbsWorkingDir, "build.bat");
        System.IO.File.WriteAllText(sAbsFilePath, sBatFileContent, Encoding.ASCII);
      }
      catch (Exception ex)
      {
        Console.WriteLine("Writing bat file failed: {0}", ex.Message);
        return false;
      }
      return true;
    }

    bool WriteBatFile(ezCMake.BuildTarget target, string sAbsWorkingDir)
    {
      try
      {
        string sBatFileContent = GetVisualStudioCommandlineInit();

        string sProjectDirectory = System.IO.Path.Combine(target.RelativePath, target.Name + ".vcxproj");
        sBatFileContent += string.Format("msbuild /p:Configuration={0};BuildProjectReferences=false /t:build {1} /flp1:logfile=errors.txt;errorsonly /flp2:logfile=warnings.txt;warningsonly /verbosity:m /clp:NoSummary\n", _settings.BuildType, sProjectDirectory);

        string sAbsFilePath = System.IO.Path.Combine(sAbsWorkingDir, "build.bat");
        System.IO.File.WriteAllText(sAbsFilePath, sBatFileContent, Encoding.ASCII);
      }
      catch (Exception ex)
      {
        Console.WriteLine("Writing bat file failed: {0}", ex.Message);
        return false;
      }
      return true;
    }

    void CleanupErrorsAndWarnings(string sAbsWorkingDir)
    {
      try
      {
        string sAbsErrorsPath = System.IO.Path.Combine(sAbsWorkingDir, "errors.txt");
        if (System.IO.File.Exists(sAbsErrorsPath))
        {
          System.IO.File.Delete(sAbsErrorsPath);
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("Deleting errors.txt failed: {0}", ex.Message);
      }

      try
      {
        string sAbsWarningsPath = System.IO.Path.Combine(sAbsWorkingDir, "warnings.txt");
        if (System.IO.File.Exists(sAbsWarningsPath))
        {
          System.IO.File.Delete(sAbsWarningsPath);
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("Deleting warnings.txt failed: {0}", ex.Message);
      }
    }

    void ReadErrorsAndWarnings(ezBuild.BuildTargetResult res, string sAbsWorkingDir)
    {
      if (res.ProcessRes == null)
        return;

      try
      {
        string sAbsErrorsPath = System.IO.Path.Combine(sAbsWorkingDir, "errors.txt");
        if (System.IO.File.Exists(sAbsErrorsPath))
        {
          string sErrors = System.IO.File.ReadAllText(sAbsErrorsPath, Encoding.UTF8);
          res.ProcessRes.ErrorOut += sErrors;
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("Reading errors.txt failed: {0}", ex.Message);
      }

      try
      {
        string sAbsWarningsPath = System.IO.Path.Combine(sAbsWorkingDir, "warnings.txt");
        if (System.IO.File.Exists(sAbsWarningsPath))
        {
          string sWarnings = System.IO.File.ReadAllText(sAbsWarningsPath, Encoding.UTF8);
          res.ProcessRes.ErrorOut += sWarnings;
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("Reading warnings.txt failed: {0}", ex.Message);
      }
    }

    #endregion Private Functions

    #region Private Members

    BuildMachineSettings _settings;
    VSVersion _Version;
    bool _bIs64Bit;

    #endregion Private Members
  }
}
