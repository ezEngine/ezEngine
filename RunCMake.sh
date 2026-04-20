#!/bin/bash -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# read arguments
opts=$(getopt \
  --longoptions help,target:,setup,force,no-unity,solution-name:,workspace-dir: \
  --name "$(basename "$0")" \
  --options "" \
  -- "$@"
)

eval set --$opts

Target="linux-gcc-debug"
NoUnity=""
SolutionName=""
WorkspaceDir=""


while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      echo "Usage: $(basename $0) [--setup] [--target <preset>] [--no-unity] [--solution-name <name>] [--workspace-dir <dir>]"
      echo "  --setup          Run first time setup. This installs dependencies and makes sure the git repository is setup correctly."
      echo "  --force          Auto confirm any prompts during setup"
      echo "  --target         CMake preset to use (e.g., linux-gcc-debug, linux-clang-dev). Default: linux-gcc-debug"
      echo "  --no-unity       Disable unity builds. This might help to improve code completion in various editors"
      echo "  --solution-name  Custom solution name"
      echo "  --workspace-dir  Custom workspace directory (outputs to Workspace/<dir> and Workspace/<dir>-output)"
      exit 0
      ;;

    --target)
      Target=$2
      shift 2
      ;;

    --setup)
      Setup=true
      shift 1
      ;;

    --force)
      Force=true
      shift 1
      ;;

    --no-unity)
      NoUnity="-DEZ_ENABLE_FOLDER_UNITY_FILES=OFF"
      shift 1
      ;;

    --solution-name)
      SolutionName=$2
      shift 2
      ;;

    --workspace-dir)
      WorkspaceDir=$2
      shift 2
      ;;

	  
    *)
      break
      ;;
  esac
done

if [ ! -f "/etc/issue" ]; then
	>&2 echo "/etc/issue does not exist. Failed distribution detection"
	exit 1
fi

Issue=$(cat /etc/issue)

UbuntuPattern="Ubuntu ([0-9][0-9])"
MintPattern="Linux Mint ([0-9][0-9])"
DebianPattern="Debian GNU/Linux ([0-9][0-9])"

if [[ $Issue =~ $UbuntuPattern ]]; then
  Distribution="Ubuntu"
  Version=${BASH_REMATCH[1]}
elif [[ $Issue =~ $MintPattern ]]; then
  Distribution="Mint"
  Version=${BASH_REMATCH[1]}
elif [[ $Issue =~ $DebianPattern ]]; then
  Distribution="Debian"
  Version=${BASH_REMATCH[1]}
fi

# This requires a 'sort' that supports '-V'
verlte() {
    [  "$1" = "`echo -e "$1\n$2" | sort -V | head -n1`" ]
}

verlt() {
    [ "$1" = "$2" ] && return 1 || verlte $1 $2
}

if [ "$Distribution" = "Ubuntu" -a \( "$Version" = "22" -o "$Version" = "24" -o "$Version" = "25" \) ] || [ "$Distribution" = "Mint" -a "$Version" = "21" ] || [ "$Distribution" = "Debian" -a "$Version" = "13" ]; then
  packages=(cmake build-essential ninja-build libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev uuid-dev mold libfreetype-dev libxkbcommon-dev liblttng-ust-dev lttng-tools)

  if [ "$Distribution" = "Ubuntu" -a \( "$Version" = "24" -o "$Version" = "25" \) ] || [ "$Distribution" = "Debian" -a "$Version" = "13" ]; then
    packages+=(libtinfo6)
  else  
    packages+=(libtinfo5)
  fi
else
  >&2 echo "Your Distribution or Distribution version is not supported by this script"
  >&2 echo "Currently supported are:"
  >&2 echo "  * Ubuntu 22"
  >&2 echo "  * Ubuntu 24"
  >&2 echo "  * Linux Mint 21"
  >&2 echo "Yours is: $Issue"
  
  exit 1
fi

if [ "$Setup" = true ]; then
  qtVer=$(apt list qt6-base-dev 2>/dev/null | grep -o "6\.[0-9]*\.[0-9]")
  echo $qtVer

  if verlt $qtVer "6.3.0"; then
    >&2 echo -e "\033[0;33mYour distributions package manager does not provide Qt 6.3.0 or newer. Please install Qt manually."
    >&2 echo -e "See https://ezengine.net/pages/docs/build/build-linux.html#automatic-setup for details.\033[0m"
  else
    packages+=(qt6-base-dev libqt6svg6-dev qt6-base-private-dev)
  fi

  # Determine compiler from target for package installation
  if [[ "$Target" == *"clang"* ]]; then
    packages+=(clang-14 libomp-14-dev libstdc++-12-dev)
  else
    packages+=(gcc-12 g++-12)
  fi

  git submodule update --init
  echo "Attempting to install the following packages through the package manager:"
  echo ${packages[@]}
  if [ "$Force" = true ]; then
    sudo apt install -y ${packages[@]}
  else
    sudo apt install ${packages[@]}
  fi

  echo -e "\nSetup complete. Run the script again without --setup to configure CMake."
  exit 0
fi

CMAKE_ARGS=("--preset" "${Target}" $NoUnity)

if [ -n "$SolutionName" ]; then
  CMAKE_ARGS+=("-DEZ_SOLUTION_NAME:STRING=${SolutionName}")
fi

# Set custom output directories to avoid conflicts between different workspaces
if [ -n "$WorkspaceDir" ]; then
  echo "Using custom workspace directory: Workspace/$WorkspaceDir"
  CMAKE_ARGS+=("-B" "$SCRIPT_DIR/Workspace/$WorkspaceDir")
  CMAKE_ARGS+=("-DEZ_OUTPUT_DIRECTORY_DLL:PATH=$SCRIPT_DIR/Workspace/$WorkspaceDir-output/Bin")
  CMAKE_ARGS+=("-DEZ_OUTPUT_DIRECTORY_LIB:PATH=$SCRIPT_DIR/Workspace/$WorkspaceDir-output/Lib")
  echo "Custom output directories: Workspace/$WorkspaceDir-output/"
fi

echo ""
echo "Running cmake ${CMAKE_ARGS[@]}"
echo ""
cmake "${CMAKE_ARGS[@]}"

# Determine build directory for ninja hint
if [ -n "$WorkspaceDir" ]; then
  BUILD_DIR="Workspace/$WorkspaceDir"
else
  BUILD_DIR="Workspace/${Target}"
fi
echo -e "\nRun 'ninja -C $BUILD_DIR' to build"
