using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BuildShared
{
  /// <summary>
  /// This class contains static helper methods for running external processes.
  /// </summary>
  public static class ezProcessHelper
  {
    /// <summary>
    /// Returned by RunExternalExe. Contains the result of the external process execution.
    /// If the application could not be started for whatever reason the ExitCode is set to -1.
    /// Additional information is available to the BuildStepResults errorHandler passed to RunExternalExe.
    /// </summary>
    public class ProcessResult
    {
      public int ExitCode { get; set; }
      public string StdOut { get; set; }
      public string ErrorOut { get; set; }
      public double Duration { get; set; }
    }

    /// <summary>
    /// Starts an external process and returns a ProcessResult with the generated output and error code.
    /// </summary>
    /// <param name="sFilename">Absolute or relative path to the executable. Must be executable from sWorkingDirectory.</param>
    /// <param name="sArguments">Arguments passed to the executable.</param>
    /// <param name="sWorkingDirectory">Working dir from which the executable is run.</param>
    /// <param name="errorHandler">Any error (except normal ErrorOut) that comes up during process execution is passed to this error handler.</param>
    /// <param name="iTimeoutMS">Timeout in milliseconds until the process is forcibly terminated.</param>
    /// <returns>Return value is never null.</returns>
    public static ProcessResult RunExternalExe(string sFilename, string sArguments, string sWorkingDirectory, BuildStepResults errorHandler, int iTimeoutMS = 10 * 60 * 1000)
    {
      OperatingSystem os = Environment.OSVersion;
      PlatformID pid = os.Platform;

      if (pid == PlatformID.Win32Windows || pid == PlatformID.Win32NT)
      {
        using (var setter = new ezWindowsErrorModeSetter())
        {
          return RunExternalExe_private(sFilename, sArguments, sWorkingDirectory, errorHandler, iTimeoutMS);
        }
      }
      else
      {
        return RunExternalExe_private(sFilename, sArguments, sWorkingDirectory, errorHandler, iTimeoutMS);
      }
    }

    private static ProcessResult RunExternalExe_private(string sFilename, string sArguments, string sWorkingDirectory, BuildStepResults errorHandler, int iTimeoutMS = 10 * 60 * 1000)
    {
      // TODO: test if file exists, or interpret thrown Win32Exception
      Stopwatch sw = new Stopwatch();
      sw.Start();

      ProcessResult res = new ProcessResult();

      Process process = new Process();

      process.StartInfo.FileName = sFilename;
      process.StartInfo.Arguments = sArguments;
      process.StartInfo.CreateNoWindow = true;
      process.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
      process.StartInfo.UseShellExecute = false;
      process.StartInfo.RedirectStandardError = true;
      process.StartInfo.RedirectStandardOutput = true;
      process.StartInfo.RedirectStandardInput = false;
      process.StartInfo.WorkingDirectory = sWorkingDirectory;

      try
      {
        process.Start();

        using (Task<bool> processWaiter = Task.Factory.StartNew(() => process.WaitForExit(iTimeoutMS)))
        using (Task<string> outputReader = Task.Factory.StartNew((Func<object, string>)ReadStream, process.StandardOutput))
        using (Task<string> errorReader = Task.Factory.StartNew((Func<object, string>)ReadStream, process.StandardError))
        {
          bool waitResult = processWaiter.Result;

          if (!waitResult)
          {
            process.Kill();
          }

          Task.WaitAll(outputReader, errorReader);

          if (!waitResult)
          {
            res.StdOut = outputReader.Result;
            res.ErrorOut = errorReader.Result;
            if (errorHandler != null)
              errorHandler.Error("Error running: {0}, {1}. The process reached its time-limit of {2} minutes!\n", sFilename, sArguments, (float)iTimeoutMS / 60000.0);
            res.ExitCode = -1;
          }
          else
          {
            res.StdOut = outputReader.Result;
            res.ErrorOut = errorReader.Result;
            res.ExitCode = process.ExitCode;
          }
        }
      }
      catch (Exception e)
      {
        if (errorHandler != null)
          errorHandler.Error("Error running: {0}, {1}. Exception: {2})", sFilename, sArguments, e.Message);
        Console.WriteLine(e);
        res.ExitCode = -1;
      }

      sw.Stop();

      res.Duration = sw.Elapsed.TotalSeconds;
      res.StdOut = (res.StdOut == null) ? "" : res.StdOut.Replace("\r", "");
      res.ErrorOut = (res.ErrorOut == null) ? "" : res.ErrorOut.Replace("\r", "");
      return res;
    }

    // Safe reading of process output from:
    // http://alabaxblog.info/2013/06/redirectstandardoutput-beginoutputreadline-pattern-broken/
    private static string ReadStream(object streamReader)
    {
      string result = ((System.IO.StreamReader)streamReader).ReadToEnd();
      return result;
    }
  }
}
