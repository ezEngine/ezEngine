using System;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace ezUwpTestHarness
{
  class ezTestUWP
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

        string _ezWorkspace;
        string _absTestOutputDirectory;
        string _configuration;
        string _platform;
        string _project;

        private ApplicationActivationManager appActiveManager;

        public ezTestUWP(string ezWorkspace, string testOutputPath,
            string configuration, string platform, string project)
        {
            _ezWorkspace = ezWorkspace;
            _absTestOutputDirectory = testOutputPath;
            _configuration = configuration;
            _platform = platform;
            _project = project;

            // May fail with an exception.
            appActiveManager = new ApplicationActivationManager();
        }

        private string GetFileserverPath()
        {
            string absBinDir = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);
            return System.IO.Path.Combine(absBinDir, "Fileserve.exe");
        }

        private void DeployAppX(out string fullPackageName)
        {
            Console.WriteLine("Deploying AppX ...");

            fullPackageName = "";

            string absSlnPath = Path.Combine(_ezWorkspace, "ezVs2017.sln");

            if (!File.Exists(absSlnPath))
            {
                throw new Exception(string.Format("Visual Studio solution '{0}' does not exist.", absSlnPath));
            }

            
            // Just hard-code the string as any other solution will break with the next VS version anyway.
            // Use ".com" version which writes into stdout
            string devEnvPath = @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.com";

            if (!File.Exists(devEnvPath))
            {
                throw new Exception(string.Format("Did not find Visual Studio installation at '{0}'.", devEnvPath));
            }

            // "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE\devenv.com" "F:\Development\current_development\ezEngine\build_uwp64\ezEngine.sln" /Deploy "RelWithDebInfo|x64" /project CoreTest
            var deployProcessResults = ezProcessHelper.RunExternalExe(devEnvPath,
                    string.Format("\"{0}\" /Deploy \"{1}|{2}\" /project {3}", absSlnPath, _configuration, _platform, _project), null);

            if (deployProcessResults.ExitCode != 0)
            {
                throw new Exception(string.Format("Deployment failed:\n{0}", deployProcessResults.StdOut));
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
                    throw new Exception(string.Format("Failed to parse full package name from Visual Studio output. Output was:\n'{0}'.", deployProcessResults.StdOut));
                }
                fullPackageName = deployProcessResults.StdOut.Substring(begin, end - begin);
            }
        }

        private void StartAppX(string fullPackageName, string args, out uint pid)
        {
            Console.WriteLine("Starting AppX ...");
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

            IntPtr errorCode = appActiveManager.ActivateApplication(appUserModelId, args, ActivateOptions.None, out pid);
            if (errorCode.ToInt64() != 0)
            {
                throw new Exception(string.Format("Activating appx '{0}' failed with error code {1}.", fullPackageName, errorCode.ToInt64()));

            }
        }

        public void ExecuteTest()
        {
            string absBinDir = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);

            //// Prepare output path (may fail, and we don't want to go through the rest if it does)
            string outputFilename = string.Format("{0}_{1}_{2}.json", _configuration, 0, _project);
            string absOutputPath = System.IO.Path.Combine(_absTestOutputDirectory, outputFilename);
            {
                System.IO.DirectoryInfo di = new DirectoryInfo(_absTestOutputDirectory);
                if (di.Exists)
                {
                    foreach (FileInfo file in di.GetFiles())
                    {
                        file.Delete();
                    }
                    foreach (DirectoryInfo dir in di.GetDirectories())
                    {
                        dir.Delete(true);
                    }
                }
                else
                {
                    System.IO.Directory.CreateDirectory(_absTestOutputDirectory);
                }
            }

            // Deploy app.
            string fullPackageName;
            DeployAppX(out fullPackageName);

            // Start fileserver.
            string absFilerserveFilename = GetFileserverPath();
            if (!File.Exists(absFilerserveFilename))
            {
                throw new Exception(string.Format("No fileserver found. File '{0}' does not exist.", absFilerserveFilename));
            }

            Func<ezProcessHelper.ProcessResult> startFileserve = () =>
            {
                Console.WriteLine("Starting Fileserve ...");



                // 60s timeout for connect, 2s timeout for closing after connection loss.
                string args = string.Format("-specialdirs eztest \"{0}\" -fs_start -fs_wait_timeout 120 -fs_close_timeout 4", _absTestOutputDirectory);
                return ezProcessHelper.RunExternalExe(absFilerserveFilename, args, absBinDir);
            };

            using (Task<ezProcessHelper.ProcessResult> runFileServe = Task.Factory.StartNew(startFileserve, System.Threading.Tasks.TaskCreationOptions.LongRunning))
            {
                runFileServe.Wait(5000);

                // Start AppX
                uint appXPid;
                StartAppX(fullPackageName, $"-json {outputFilename} -nogui -rev 0 -all", out appXPid);
                
                Process appXProcess;
                appXProcess = Process.GetProcessById((int)appXPid);

                Task.WaitAll(runFileServe);

                // Check whether the AppX is dead by now.
                if (!appXProcess.WaitForExit(5000))
                {
                    Console.WriteLine(string.Format("Fileserve is no longer running but the AppX is."));
                    try
                    {
                        string args = string.Format("-ma {0}", appXPid);
                        ezProcessHelper.RunExternalExe("procdump", args, absBinDir);
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine(string.Format("Failed to run procdump: '{0}'", e.ToString()));
                    }
                    appXProcess.Kill();
                }
                // Can't read exit code: "Process was not started by this object, so requested information cannot be determined"
                appXProcess.Dispose();
                appXProcess = null;
            }

            // Read test output.
            if (File.Exists(absOutputPath))
            {
                string TestResultJSON = File.ReadAllText(absOutputPath, Encoding.UTF8);

                try
                {
                    // Parse test output to figure out what the result is as we can't use the exit code.
                    var values = Newtonsoft.Json.JsonConvert.DeserializeObject<System.Collections.Generic.Dictionary<string, dynamic>>(TestResultJSON);
                    var errors = values["errors"] as Newtonsoft.Json.Linq.JArray;
                    Console.WriteLine(string.Format("Errors during test: '{0}'", errors.Count));
                    Environment.ExitCode = errors.Count;
                }
                catch (Exception e)
                {
                    Environment.ExitCode = 1;
                    Console.WriteLine(string.Format("Failed to parse test output: '{0}'", e.ToString()));
                }
            }
            else
            {
                Environment.ExitCode = 1;
                Console.WriteLine(string.Format("No test output file present!"));
            }
        }
    }

}
