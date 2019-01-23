using BuildShared;

namespace BuildMachine
{
  /// <summary>
  /// Static class that contains the test template factory.
  /// </summary>
  public static class ezTestTemplateFactory
  {
    /// <summary>
    /// Returns the test template for the given configuration.
    /// </summary>
    /// <param name="sConfiguration">Configuration as returned by CMake.</param>
    /// <returns></returns>
    public static ezTestTemplate Create(BuildMachineSettings settings)
    {
      if (settings.Configuration.StartsWith("WinUWP"))
      {
        return new ezTestUWP();
      }
      else
      {
        return new ezTestDefault();
      }
    }
  }

  /// <summary>
  /// The test template interface that abstracts the test process.
  /// </summary>
  public abstract class ezTestTemplate
  {
    protected string GetOutputFileName(ezCMake.TestTarget target, BuildMachineSettings settings)
    {
      return string.Format("{0}_{1}_{2}.json", settings.Configuration, settings.Revision, target.Name);
    }

    /// <summary>
    /// In case we are re-running a build process the output file may already exist and we need to make sure it doesn't contaminate the new test run.
    /// This function deletes the output file if it exists.
    /// </summary>
    protected bool DeleteOutputFile(string absOutputPath, ref ezTest.TestTargetResult res)
    {
      if (System.IO.File.Exists(absOutputPath))
      {
        try
        {
          System.IO.File.Delete(absOutputPath);
        }
        catch (System.Exception ex)
        {
          res.Error("Error deleting test output file ({0}): {1}", absOutputPath, ex.Message);
          return false;
        }
      }
      return true;
    }

    static protected string GetDefaultTestArgs(string outputFilename, BuildMachineSettings settings)
    {
      return string.Format("-json \"{0}\" -rev {1} -nogui -all", outputFilename, settings.Revision);
    }

    public abstract ezTest.TestTargetResult BuildTarget(ezCMake.TestTarget target, BuildMachineSettings settings);
  }
}
