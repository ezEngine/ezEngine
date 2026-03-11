#!/bin/sh

if [ $(cat /proc/sys/kernel/yama/ptrace_scope) = "1" ]
then
  /usr/bin/pkexec --disable-internal-agent /usr/bin/bash -c "echo 0 > /proc/sys/kernel/yama/ptrace_scope"
fi
