using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using BuildShared;

namespace BuildMachine
{
  /// <summary>
  /// Build template implementation for the Visual D compiler project.
  /// </summary>
  public class ezBuildWinD : ezBuildTemplate
  {
    public ezBuildWinD(BuildMachineSettings settings, VSVersion version, bool bIs64Bit)
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

      res.ProcessRes = ezProcessHelper.RunExternalExe("cmd", "/c build.bat", sAbsWorkingDir, res);
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
        default:
          return "";
      }
    }

    bool WriteBatFileCleanSolution(string sAbsWorkingDir)
    {
      try
      {
        string sBatFileContent = GetVisualStudioCommandlineInit();
        sBatFileContent += string.Format("devenv /clean {0} \"ezEngine.sln\"\r\n", _settings.BuildType);

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
        sBatFileContent += string.Format("devenv /build {0} /project {1} \"ezEngine.sln\"\r\n", _settings.BuildType, target.Name);

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

    #endregion Private Functions

    #region Private Members

    BuildMachineSettings _settings;
    VSVersion _Version;
    bool _bIs64Bit;

    #endregion Private Members
  }
}
