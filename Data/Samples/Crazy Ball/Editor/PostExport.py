# Example for a post-export script. The editor runs it after every project export of this sample.
# See the 'Project Export' documentation page for details.
#
# The export writes one launch script per scene, but players should always start in the main scene.
# So this keeps only that one and gives it a name that makes sense to a player.

import os
import pathlib
import sys

export_dir = pathlib.Path(os.environ["EZ_EXPORT_DIR"])

main_launcher = export_dir / "Launch Main.bat"

if not main_launcher.exists():
    # A non-zero exit code makes the export fail, which is better than shipping a package nobody can start.
    print(f"Launch script not found: {main_launcher}", file=sys.stderr)
    sys.exit(1)

main_launcher.rename(export_dir / "Play Crazy Ball.bat")

for launcher in export_dir.glob("Launch *.bat"):
    launcher.unlink()
    print(f"Removed {launcher.name}")

# Everything that is printed ends up in the export log.
print("Created 'Play Crazy Ball.bat'")
