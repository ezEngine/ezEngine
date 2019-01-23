using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;

namespace UnityCtrlF7
{
  [PackageRegistration(UseManagedResourcesOnly = true)]
  [ProvideAutoLoad(UIContextGuids80.SolutionExists)]
  [ProvideMenuResource("Menus.ctmenu", 1)]

  [Guid(UnityCtrlF7Package.PackageGuidString)]
  [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "pkgdef, VS and vsixmanifest are valid VS terms")]
  [InstalledProductRegistration("#110", "#112", "0.2", IconResourceID = 400)]
  public sealed class UnityCtrlF7Package : Package
  {
    /// <summary>
    /// UnityCtrlF7Package GUID string.
    /// </summary>
    public const string PackageGuidString = "A2194A9C-931C-455E-A1D5-56C8123B36E1";

    public UnityCtrlF7Package()
    {
      // Inside this method you can place any initialization code that does not require
      // any Visual Studio service because at this point the package object is created but
      // not sited yet inside Visual Studio environment. The place to do all the other
      // initialization is the Initialize method.
    }

    #region Package Members

    /// <summary>
    /// Initialization of the package; this method is called right after the package is sited, so this is the place
    /// where you can put all the initialization code that rely on services provided by VisualStudio.
    /// </summary>
    protected override void Initialize()
    {
      Commands.CompileUnityFile_CodeWindow.Initialize(this);

      base.Initialize();
    }

    protected override void Dispose(bool disposing)
    {
      base.Dispose(disposing);
    }

    #endregion
  }
}
