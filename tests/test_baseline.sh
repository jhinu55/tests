#!/bin/sh
set -eu
rm -f /dev/mqueue/osdoc_* output.txt
./build/server >tests/server.log 2>&1 &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true; wait "$server_pid" 2>/dev/null || true' EXIT
sleep 0.1
./build/client 0 >tests/client.log
grep -q 'OK 0 0: Hello' tests/client.log
grep -q 'OK 0 1: World' tests/client.log
kill -INT "$server_pid"
wait "$server_pid"
test ! -e /dev/mqueue/osdoc_server
echo "baseline tests passed"
