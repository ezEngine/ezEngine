using BuildShared;
using System;
using System.Text;
using System.IO;
using System.Diagnostics;
using Microsoft.Win32;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace BuildMachine
{
  class ezTestUWP : ezTestTemplate
  {
    #region Activation Manager COM definitions

    // From: http://stackoverflow.com/questions/12925748/iapplicationactivationmanageractivateapplication-in-c

    public enum ActivateOptions
    {
      None = 0x00000000,  // No flags set
      DesignMode = 0x00000001,  // The application is being activated for design mode, and thus will not be able to
                                // to create an immersive window. Window creation must be done by design tools which
                                // load the necessary components by communicating with a designer-specified service on
                                // the site chain established on the activation manager.  The splash screen normally
                                // shown when an application is activated will also not appear.  Most activations
                                // will not use this flag.
      NoErrorUI = 0x00000002,  // Do not show an error dialog if the app fails to activate.                                
      NoSplashScreen = 0x00000004,  // Do not show the splash screen when activating the app.
    }

    [ComImport, Guid("2e941141-7f97-4756-ba1d-9decde894a3d"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IApplicationActivationManager
    {
      // Activates the specified immersive application for the "Launch" contract, passing the provided arguments
      // string into the application.  Callers can obtain the process Id of the application instance fulfilling this contract.
      IntPtr ActivateApplication([In] String appUserModelId, [In] String arguments, [In] ActivateOptions options, [Out] out UInt32 processId);
      IntPtr ActivateForFile([In] String appUserModelId, [In] IntPtr /*IShellItemArray* */ itemArray, [In] String verb, [Out] out UInt32 processId);
      IntPtr ActivateForProtocol([In] String appUserModelId, [In] IntPtr /* IShellItemArray* */itemArray, [Out] out UInt32 processId);
    }

    [ComImport, Guid("45BA127D-10A8-46EA-8AB7-56EA9078943C")]//Application Activation Manager
    class ApplicationActivationManager : IApplicationActivationManager
    {
      [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)/*, PreserveSig*/]
      public extern IntPtr ActivateApplication([In] String appUserModelId, [In] String arguments, [In] ActivateOptions options, [Out] out UInt32 processId);
      [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
      public extern IntPtr ActivateForFile([In] String appUserModelId, [In] IntPtr /*IShellItemArray* */ itemArray, [In] String verb, [Out] out UInt32 processId);
      [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
      public extern IntPtr ActivateForProtocol([In] String appUserModelId, [In] IntPtr /* IShellItemArray* */itemArray, [Out] out UInt32 processId);
    }

    #endregion

    static readonly string relativeFileservePath = "Data/Tools/Precompiled/Fileserve.exe";
    static readonly string relativeTestDataPath = "Data/EditorSamples/Testing Chambers";
    //static readonly string VSLauncherLocation = "%ProgramFiles(x86)%/Common Files/Microsoft Shared/MSEnv/VSLauncher.exe";
    private ApplicationActivationManager appActiveManager;

    public ezTestUWP()
    {
      // May fail with an exception.
      appActiveManager = new ApplicationActivationManager();
    }

    static private string GetFileserverPath(BuildMachineSettings settings)
    {
      return Path.Combine(settings.AbsCodePath, relativeFileservePath);
    }

    private BuildStepResults DeployAppX(ezCMake.TestTarget target, BuildMachineSettings settings, out string fullPackageName)
    {
      Console.WriteLine("Deploying AppX ...");

      BuildStepResults result = new BuildStepResults();
      fullPackageName = "";

      string absSlnPath = Path.Combine(settings.AbsCMakeWorkspace, "ezEngine.sln");
      if (!File.Exists(absSlnPath))
      {
        result.Error("Visual Studio solution '{0}' does not exist.", absSlnPath);
        return result;
      }


      // VSLauncher vs using devenv.exe directly.
      // 
      // Pro VSLauncher:
      // - It picks always the appropriate VS version
      // - We know more certainly where it is
      //
      // Con VSLauncher:
      // - Spawns devenv.exe and closes again (we don't know when devenv.exe finishes or if it came up in the first place)
      // - No console output

      //string VSLauncherAbsPath = Environment.ExpandEnvironmentVariables(VSLauncherLocation);
      //if (!File.Exists(VSLauncherAbsPath))
      //{
      //  result.Error("Did not find Visual Studio launcher at '{0}'.", VSLauncherAbsPath);
      //  return result;
      //}

      // Using this registry key we should always get the newest devenv version.
      // Since newer versions can use old compilers & SDKs this should be perfectly fine.
      // https://social.msdn.microsoft.com/Forums/vstudio/en-US/568e32af-d724-4ac6-8e8f-72181c4320b3/set-default-version-of-visual-studio?forum=vssetup
      string devEnvPath;
      try
      {
        using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\App Paths\devenv.exe"))
        {
          if (key != null)
          {
            devEnvPath = key.GetValue("") as string;
            if (devEnvPath == null)
            {
              result.Error("Failed to read Visual Studio location from registry key: No string value in key found.");
              return result;
            }
          }
          else
          {
            result.Error("Failed to read Visual Studio location from registry key: Registry key not found.");
            return result;
          }
        }
      }
      catch (Exception e)
      {
        result.Error("Failed to read Visual Studio location from registry key: {0}", e);
        return result;
      }

      // Use ".com" version which writes into stdout
      devEnvPath = devEnvPath.Replace("devenv.exe", "devenv.com");

      if (!File.Exists(devEnvPath))
      {
        result.Error("Did not find Visual Studio installation at '{0}'.", devEnvPath);
        return result;
      }

      // "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE\devenv.com" "F:\Development\current_development\ezEngine\build_uwp64\ezEngine.sln" /Deploy "RelWithDebInfo|x64" /project CoreTest

      string platform = settings.Configuration.EndsWith("64") ? "x64" : "Win32";  // No ARM support yet.
      var deployProcessResults = ezProcessHelper.RunExternalExe(devEnvPath, 
              string.Format("\"{0}\" /Deploy \"{1}|{2}\" /project {3}", absSlnPath, settings.BuildType, platform, target.Name), null, result);

      result.Duration = deployProcessResults.Duration;
      if (deployProcessResults.ExitCode != 0)
      {
        result.Error("Deployment failed:\n{0}", deployProcessResults.StdOut);
        result.Success = false;
      }
      else
      {
        // Get full package name from deploy output.
        // From the build configuration we only know the package name, not the full identifier. This little parse saves us from searching the package registry.
        string fullPackageNameStartString = "Full package name: \"";
        int begin = deployProcessResults.StdOut.LastIndexOf(fullPackageNameStartString) + fullPackageNameStartString.Length;
        int end = deployProcessResults.StdOut.IndexOf("\"", begin);
        if (begin < 0 || end < 0)
        {
          result.Error("Failed to parse full package name from Visual Studio output. Output was:\n'{0}'.", deployProcessResults.StdOut);
          return result;
        }
        fullPackageName = deployProcessResults.StdOut.Substring(begin, end - begin);

        result.Success = true;
      }

      return result;
    }

    private BuildStepResults StartAppX(string fullPackageName, string args, out uint pid)
    {
      Console.WriteLine("Starting AppX ...");

      BuildStepResults res = new BuildStepResults();
      pid = 0;


      // Need "App user model id". Clean ways of doing this...
      // * Read AppX Manifest to find out the user model id (is there an easier way that is also not based on hacking?)
      // * Could at least extract the family name instead of guessing
      // * ??
      // -> In any (known) case we would need something like Windows.Management.Deployment.PackageManager.FindPackage which brings WinRT dependencies into this project.
      //
      // The following method seems to work just fine for ezEngine projects if not for all AppX in general.
      //
      // From experience (documentation link?) we know that the user model id is the AppX family name + "!" + entry point.
      // We also know, that the family name is like the fullPackageName without version and platform numbers.
      // The AppX name (first part) and the suffix can't contain underscores, so it can safely be split with '_'
      var packageNameParts = fullPackageName.Split('_');
      string appFamilyName = packageNameParts[0] + "_" + packageNameParts[packageNameParts.Length - 1];
      string appUserModelId = appFamilyName + "!App"; // Apply our knowledge of ezEngine's appx manifests.

      try
      {
        IntPtr errorCode = appActiveManager.ActivateApplication(appUserModelId, args, ActivateOptions.None, out pid);
        if (errorCode.ToInt64() != 0)
        {
          res.Error("Activating appx '{0}' failed with error code {1}.", fullPackageName, errorCode.ToInt64());
          return res;
        }
      }
      catch (Exception e)
      {
        res.Error("Failed to activate appx: {0}", e);
        return res;
      }

      res.Success = true;
      return res;
    }

    public override ezTest.TestTargetResult BuildTarget(ezCMake.TestTarget target, BuildMachineSettings settings)
    {
      ezTest.TestTargetResult res = new ezTest.TestTargetResult();
      res.Name = target.Name;
      res.NeedsHardwareAccess = target.NeedsHardwareAccess;
      res.Experimental = target.Experimental;


      // Prepare output path (may fail, and we don't want to go through the rest if it does)
      string outputFilename = GetOutputFileName(target, settings);
      string absOutputPath = System.IO.Path.Combine(settings.AbsOutputFolder, outputFilename);
      if (!DeleteOutputFile(absOutputPath, ref res))
        return res;

      // Deploy app.
      string fullPackageName;
      var deployResult = DeployAppX(target, settings, out fullPackageName);
      res.MergeIn(deployResult);
      if (!deployResult.Success)
        return res;

      // Start AppX
      uint appXPid;
      var startResult = StartAppX(fullPackageName, GetDefaultTestArgs(outputFilename, settings), out appXPid);
      res.MergeIn(startResult);
      if (!startResult.Success)
        return res;
      Process appXProcess;
      try
      {
        appXProcess = Process.GetProcessById((int)appXPid);
      }
      catch (Exception e)
      {
        res.Error("Failed to get process handle to test app: {0}", e);
        return res;
      }

      // Start fileserver.
      string absFilerserveFilename = GetFileserverPath(settings);
      if (!File.Exists(absFilerserveFilename))
      {
        res.Error("No fileserver found. File '{0}' does not exist.", absFilerserveFilename);
      }
      else
      {
        string absBinDir = Path.Combine(settings.AbsBinPath, settings.Configuration);
        string absTestDataPath = Path.Combine(settings.AbsCodePath, relativeTestDataPath);
        // 20s timeout for connect, 2s timeout for closing after connection loss.
        string args = string.Format("-specialdirs project \"{0}\" eztest \"{1}\" -fs_start -fs_wait_timeout 20 -fs_close_timeout 2", absTestDataPath, settings.AbsOutputFolder);
        res.ProcessRes = ezProcessHelper.RunExternalExe(absFilerserveFilename, args, absBinDir, res);
        res.Duration += res.ProcessRes.Duration;
        res.Success = (res.ProcessRes.ExitCode == 0);
      }

      // Top watch.

      // Check whether the AppX is dead by now.
      if(!appXProcess.HasExited)
      {
        res.Error("Fileserve is no longer running but the AppX is.");
        res.Success = false;
        appXProcess.Kill();
      }
      // Can't read exit code: "Process was not started by this object, so requested information cannot be determined"
      /*else
      {
        if (appXProcess.ExitCode != 0)
        {
          res.Error("Test AppX exited with {0}", appXProcess.ExitCode);
          res.Success = false;
        }
      }*/
      appXProcess.Dispose();
      appXProcess = null;

      // Read test output.
      if (File.Exists(absOutputPath))
      {
        res.TestResultJSON = File.ReadAllText(absOutputPath, Encoding.UTF8);
        //TODO: use test json output as stdout substitute until pipes are implemented.
        res.ProcessRes.StdOut = res.TestResultJSON;
        try
        {
          // Parse test output to figure out what the result is as we can't use the exit code.
          var values = Newtonsoft.Json.JsonConvert.DeserializeObject<System.Collections.Generic.Dictionary<string, dynamic>>(res.TestResultJSON);
          var errors = values["errors"] as Newtonsoft.Json.Linq.JArray;
          res.Success = errors.Count == 0;
        }
        catch (Exception e)
        {
          res.Success = false;
          res.Error("Failed to parse test output: '{0}'", e.ToString());
        }
      }
      else
      {
        res.Error("No output file present!");
        res.Success = false;
      }

      if (!res.Success && !res.Experimental)
        res.Error("Testing '{0}' failed!", res.Name);
      return res;
    }
  }
}
