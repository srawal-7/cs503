#!/usr/bin/env bats

@test "Check for simple command execution" {
    run ./dsh <<EOF
echo "Hello, World!"
EOF

    [ "$status" -eq 0 ]
    [[ "$output" =~ "Hello, World!" ]]
}

@test "Check 'pipes with multiple arguments' works" {
    run "./dsh" <<EOF
ls | grep dshlib.c | wc -w
EOF

    # Strip all whitespace (spaces, tabs, newlines) from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Expected output with all whitespace removed for easier matching
    expected_output="dshlib.clocalmodedsh4>dsh4>cmdloopreturned0"

    # These echo commands will help with debugging and will only print
    #if the test fails
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    # Check exact match
    [ "$stripped_output" = "$expected_output" ]

    # Assertions
    [ "$status" -eq 0 ]
}


@test "Check 'Redirection: output to file' works" {
    run ./dsh <<EOF
echo "Test redirection" > test_output.txt
EOF

    [ "$status" -eq 0 ]
    [ -f test_output.txt ]
    grep -q "Test redirection" test_output.txt
    rm test_output.txt # Cleanup
}

@test "Check 'cd' changes directory" {
    run ./dsh <<EOF
cd /
pwd
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == "/"* ]]
}

@test "Check exit command works" {
    run ./dsh <<EOF
exit
EOF

    [ "$status" -eq 0 ]
}

#################################
### Functions for client-server checks
#### Denoted as remote

# Set the IP and port for the remote shell server
REMOTE_IP="127.0.0.1"
REMOTE_PORT="1234"

# Start the server in the background before running tests
setup() {
    pgrep dsh > /dev/null || ./dsh -s -i $REMOTE_IP -p $REMOTE_PORT &
    sleep 2 # Give server time to start
}

# Ensure the server is stopped after testing
teardown() {
    ./dsh -c -i $REMOTE_IP -p $REMOTE_PORT <<EOF
stop-server
EOF
    sleep 1 # Give server time to stop
}

@test "Remote: Server start and client connect check" {
  ./dsh -s -i $REMOTE_IP -p $REMOTE_PORT & # Start server in background
  sleep 1 # Wait for server to start

  run ./dsh -c -i $REMOTE_IP -p $REMOTE_PORT <<EOF
exit
EOF

  [ "$status" -eq 0 ]
}

@test "Remote: Simple command execution check" {
    run ./dsh -c -i $REMOTE_IP -p $REMOTE_PORT <<EOF
echo "Hello from remote"
EOF

    [ "$status" -eq 0 ]
    [[ "$output" =~ "Hello from remote" ]]
}

@test "Remote: List files (ls) check" {
    run ./dsh -c -i $REMOTE_IP -p $REMOTE_PORT <<EOF
ls
EOF

    [ "$status" -eq 0 ]
    [[ "$output" =~ "dsh" ]] 
}

@test "Remote: Output redirection check" {
  ./dsh -s -i $REMOTE_IP -p $REMOTE_PORT & # Start server in background
  sleep 1 # Wait for server to start

  run ./dsh -c -i $REMOTE_IP -p $REMOTE_PORT <<EOF
echo "Test redirection" > remote_test.txt
cat remote_test.txt
rm remote_test.txt
exit
EOF

  [ "$status" -eq 0 ]
  [[ "$output" =~ "Test redirection" ]]
}

@test "Remote: 'pwd' command check" {
  ./dsh -s -i $REMOTE_IP -p $REMOTE_PORT & # Start server in background
  sleep 1 # Wait for server to start

  run ./dsh -c -i $REMOTE_IP -p $REMOTE_PORT <<EOF
pwd
exit
EOF

  [ "$status" -eq 0 ]
  [[ "$output" =~ "/" ]]
}

@test "Remote: Stop server check" {
    run ./dsh -c -i $REMOTE_IP -p $REMOTE_PORT <<EOF
stop-server
EOF

    [ "$status" -eq 0 ]
}