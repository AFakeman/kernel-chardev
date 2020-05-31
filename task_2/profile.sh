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
# Erasing existing trace log
echo > /sys/kernel/debug/tracing/trace
# Starting the trace

echo 1 > /sys/kernel/debug/tracing/tracing_on
cat /dev/thread_test_dev 2>/dev/null || true
for i in {1..1000}; do
    cat /dev/thread_test_dev 2>/dev/null || true;
done
echo 0 > /sys/kernel/debug/tracing/tracing_on

rmmod $1
sysctl kernel.ftrace_enabled=0 >/dev/null
cat /sys/kernel/debug/tracing/trace | grep -v '^#' |
    awk '
    function to_us(time, units) {
        if (units == "ms") {
            return time * 1000
        }
        if (units == "s") {
            return time * 1000000
        }
        if (units == "ns") {
            return time / 1000
        }
        return time
    }
    {
        if ($1 ~ /^$/) {
            next
        }
        if ($1 ~ /------/) {
            next
        }
        if ($3 ~ /=>/) {
            if (line == 2) {
                spinlock += spinlock_one
                mutex += mutex_one
                count += 1
            }
            line = 0
            next
        }
        if (line == 0) {
            spinlock_one = to_us($3, $4)
        } else if (line == 1) {
            mutex_one = to_us($3, $4)
        }
        line += 1
    }
    END {
        if (line == 2) {
            spinlock += spinlock_one
            mutex += mutex_one
        }
        print "Iterations: " count
        print "Spinlock: " spinlock / count
        print "Mutex: " mutex / count
    }
'
