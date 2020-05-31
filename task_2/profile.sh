#!/bin/bash
set -e

failure() {
  local lineno=$1
  local msg=$2
  echo "Failed at $lineno: $msg"
}
trap 'failure ${LINENO} "$BASH_COMMAND"' ERR

# In order to trace our function, we need to insert it.
insmod $1.ko
sysctl kernel.ftrace_enabled=1 >/dev/null
echo function_graph > /sys/kernel/debug/tracing/current_tracer
echo run_test > /sys/kernel/debug/tracing/set_ftrace_filter
rmmod $1
# Erasing existing trace log
echo > /sys/kernel/debug/tracing/trace
# Starting the trace
echo 1 > /sys/kernel/debug/tracing/tracing_on
# I have no idea why exacly, but I have to insert the module twice,
# otherwise trace will be empty.
insmod $1.ko
rmmod $1
insmod $1.ko
rmmod $1
# Stopping the trace
echo 0 > /sys/kernel/debug/tracing/tracing_on
LINES=$(cat /sys/kernel/debug/tracing/trace | grep -v '^#' | cut -d' ' -f4,5)
echo -n "Spinlock: "
echo "$LINES" | sed -n 1p
echo -n "Mutex: "
echo "$LINES" | sed -n 2p
sysctl kernel.ftrace_enabled=0 >/dev/null
