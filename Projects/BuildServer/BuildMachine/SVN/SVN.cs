using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using BuildShared;

namespace BuildMachine
{
  /// <summary>
  /// This class takes care of the SVN update in a build process.
  /// </summary>
  public class ezSVN
  {
    public ezSVN()
    {
    }

    public bool Init(BuildMachineSettings settings)
    {
      _Settings = settings;

      try
      {
        // Make sure the given parameter is valid.
        if (!System.IO.Path.IsPathRooted(_Settings.AbsCodePath))
        {
          Console.WriteLine("SVN Init failed: The path '{0}' is not absolute!", _Settings.AbsCodePath);
          return false;
        }

        if (!System.IO.Directory.Exists(_Settings.AbsCodePath))
        {
          Console.WriteLine("SVN Init failed: The path '{0}' does not exist!", _Settings.AbsCodePath);
          return false;
        }

        Console.WriteLine("SVN Init: Code path: '{0}'.", _Settings.AbsCodePath);
      }
      catch (Exception ex)
      {
        Console.WriteLine(ex);
        return false;
      }
      return true;
    }

    public SVNResult Run(int iRevision)
    {
      _Result.Clean();

      Console.WriteLine("*** Starting SVN update to rev {0} ***", iRevision);
      try
      {
        // Run SVN update
        string sParams;
        if (iRevision < 0)
          sParams = string.Format("update --username {0} --password {1}" /*tf"*/, _Settings.SVNUsername, _Settings.SVNPassword);
        else
          sParams = string.Format("update -r {0} --username {1} --password {2}" /*tf"*/, iRevision, _Settings.SVNUsername, _Settings.SVNPassword);

        for (int iTry = 0; iTry < 2; ++iTry)
        {
          _Result.ProcessRes = ezProcessHelper.RunExternalExe("svn", sParams, _Settings.AbsCodePath, _Result);
          _Result.ProcessRes.StdOut = _Result.ProcessRes.StdOut.Replace(_Settings.SVNPassword, "***").Replace(_Settings.SVNUsername, "***");
          _Result.ProcessRes.ErrorOut = _Result.ProcessRes.ErrorOut.Replace(_Settings.SVNPassword, "***").Replace(_Settings.SVNUsername, "***");
          _Result.Errors = _Result.Errors.Replace(_Settings.SVNPassword, "***").Replace(_Settings.SVNUsername, "***");

          if (_Result.ProcessRes.ExitCode != 0)
          {
            if (iTry == 0 && (_Result.ProcessRes.StdOut.Contains("lock") || _Result.ProcessRes.ErrorOut.Contains("lock")))
            {
              _Result.Error("SVN repository locked: trying cleanup...");
              ezProcessHelper.RunExternalExe("svn", "cleanup", _Settings.AbsCodePath, _Result);
              continue;
            }
            else
            {
              _Result.Error("SVN failed: SVN returned ErrorCode: {0}!", _Result.ProcessRes.ExitCode);
              _Result.Success = false;
              return _Result;
            }
          }
          else
          {
            break;
          }
        }
        _Result.Duration = _Result.ProcessRes.Duration;
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

    #endregion Private Functions

    /// <summary>
    /// The results of the SVN update step in the build process.
    /// </summary>
    public class SVNResult : BuildStepResults
    {
      public ezProcessHelper.ProcessResult ProcessRes { get; set; }

      public override void Clean()
      {
        base.Clean();
        ProcessRes = null;
      }
    }

    #region Private Members

    SVNResult _Result = new SVNResult();
    BuildMachineSettings _Settings = null;

    #endregion Private Members
  }
}
