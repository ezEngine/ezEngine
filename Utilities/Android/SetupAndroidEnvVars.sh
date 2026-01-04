#!/usr/bin/env bash
# SetupAndroidEnvVars.sh
# Source this script to export Android-related environment variables for the repo.
# Usage: source Utilities/Android/SetupAndroidEnvVars.sh

set -euo pipefail

# Detect if script is sourced
sourced=0
if [ -n "${BASH_SOURCE-+x}" ]; then
  if [ "${BASH_SOURCE[0]}" != "$0" ]; then
    sourced=1
  fi
fi

# Fallback check for shells that support return
if [ $sourced -eq 0 ]; then
  (return 0 2>/dev/null) && sourced=1 || true
fi

if [ $sourced -ne 1 ]; then
  echo "Please source this file to export environment variables:"
  echo "  source ${BASH_SOURCE[0]}"
  exit 1
fi

# Determine repository root (always two levels up from this script)
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Defaults matching the pipeline layout
ANDROID_HOME="$REPO_ROOT/Workspace/shared/android"
ANDROID_NDK_HOME="$ANDROID_HOME/ndk/26.1.10909125"
JAVA_HOME="$ANDROID_HOME/jdk17"

export ANDROID_HOME ANDROID_NDK_HOME JAVA_HOME

# Add platform-tools (adb) to PATH if not already present
platform_tools="$ANDROID_HOME/platform-tools"
case ":$PATH:" in
  *":$platform_tools:") ;;
  *) export PATH="$platform_tools:$PATH" ;;
esac

cat <<EOF
Exported environment variables:
  ANDROID_HOME=$ANDROID_HOME
  ANDROID_NDK_HOME=$ANDROID_NDK_HOME
  JAVA_HOME=$JAVA_HOME
  PATH=$PATH
EOF
