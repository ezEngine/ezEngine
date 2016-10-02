using System;
using System.Linq;
using System.Threading;
using System.Windows;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.VCProjectEngine;
using EnvDTE;
using Microsoft.VisualStudio.Text;

namespace UnityCtrlF7
{
  /// <summary>
  /// Command handler
  /// </summary>
  internal sealed class CompileUnityFile
  {
    public static bool WorkInProgress { get; private set; }
    private AutoResetEvent outputWaitEvent = new AutoResetEvent(false);
    private const int timeoutMS = 30000; // 30 seconds

    public static VCFileConfiguration GetFileConfig(EnvDTE.Document document, out string reasonForFailure, out bool isHeader)
    {
      isHeader = false;

      if (document == null)
      {
        reasonForFailure = "No document.";
        return null;
      }

      string unityPath = document.Path;
      unityPath = System.IO.Path.Combine(unityPath, "unity");

      var project = document.ProjectItem?.ContainingProject;
      VCProject vcProject = project.Object as VCProject;
      if (vcProject == null)
      {
        reasonForFailure = "The given document does not belong to a VC++ Project.";
        return null;
      }

      VCFile vcFile = document.ProjectItem?.Object as VCFile;
      if (vcFile == null)
      {
        reasonForFailure = "The given document is not a VC++ file.";
        return null;
      }
      VCFilter vcFolder = vcFile.Parent as VCFilter;
      if (vcFolder == null)
      {
        reasonForFailure = "Can't get folder for VC++ file.";
        return null;
      }


      // Replace file with unity file with the same path
      foreach (VCFile file in vcFolder.Items)
      {
        if (file != null)
        {
          if (System.IO.Path.GetFileName(file.FullPath).StartsWith("unity_"))
          {
            vcFile = file;
            break;
          }
        }
      }

      if (vcFile == null)
      {
        reasonForFailure = "The given document is not a VC++ file.";
        return null;
      }

      isHeader = vcFile.FileType == Microsoft.VisualStudio.VCProjectEngine.eFileType.eFileTypeCppHeader;

      IVCCollection fileConfigCollection = vcFile.FileConfigurations;
      VCFileConfiguration fileConfig = fileConfigCollection?.Item(vcProject.ActiveConfiguration.Name);
      if (fileConfig == null)
      {
        reasonForFailure = "Failed to retrieve file config from document.";
        return null;
      }

      reasonForFailure = "";
      return fileConfig;
    }


    public void PerformCompileUnityFile(EnvDTE.Document document)
    {
      if (document == null)
        return;

      string errorMessage;
      bool isHeader;
      var fileConfig = GetFileConfig(document, out errorMessage, out isHeader);
      if (fileConfig == null)
      {
        Output.Instance.WriteLine(errorMessage);
        return;
      }
      if (isHeader)
      {
        return;
      }

      PerformCompileUnityFile(document, fileConfig);
    }

    public void PerformCompileUnityFile(EnvDTE.Document document, VCFileConfiguration fileConfig)
    {
      if (document == null || fileConfig == null)
        return;

      if (WorkInProgress)
      {
        return;
      }
      WorkInProgress = true;

      // Hook into build events.
      document.DTE.Events.BuildEvents.OnBuildProjConfigDone += OnBuildConfigFinished;
      document.DTE.Events.BuildEvents.OnBuildDone += OnBuildFinished;

      // The rest runs in a separate thread since the compile function is non blocking and we want to use BuildEvents
      // We are not using Task, since we want to make use of WaitHandles - using this together with Task is a bit more complicated to get right.
      outputWaitEvent.Reset();
      new System.Threading.Thread(() =>
      {
        try
        {
          // Compile - In rare cases VS tells us that we are still building which should not be possible because we have received OnBuildFinished
          // As a workaround we just try again a few times.
          {
            const int maxNumCompileAttempts = 1;
            for (int numCompileFails = 0; numCompileFails < maxNumCompileAttempts; ++numCompileFails)
            {
              try
              {
                fileConfig.Compile(true, false); // WaitOnBuild==true always fails.    
              }
              catch (Exception e)
              {
                if (numCompileFails == maxNumCompileAttempts - 1)
                {
                  Output.Instance.WriteLine("Compile Failed:\n{0}", e);

                  throw e;
                }
                else
                {
                  // Try again.
                  System.Threading.Thread.Sleep(100);
                  continue;
                }
              }
              break;
            }
          }

          // Wait till woken.
          bool noTimeout = outputWaitEvent.WaitOne(timeoutMS);
        }
        catch (Exception ex)
        {
          Output.Instance.WriteLine("Unexpected error: {0}", ex);
        }
        finally
        {
          Application.Current.Dispatcher.Invoke(() =>
                {
                  // Remove build hook again.
                  document.DTE.Events.BuildEvents.OnBuildDone -= OnBuildFinished;
                  document.DTE.Events.BuildEvents.OnBuildProjConfigDone -= OnBuildConfigFinished;

                  // Message.
                  //Output.Instance.OutputToForeground();

                  // Notify that we are done.
                  WorkInProgress = false;
                });
        }
      }).Start();
    }

    private void OnBuildFinished(vsBuildScope scope, vsBuildAction action)
    {
      outputWaitEvent.Set();
    }

    private void OnBuildConfigFinished(string project, string projectConfig, string platform, string solutionConfig, bool success)
    {
    }
  }
}