#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Pipes with three arguments" {
    run "./dsh" <<EOF                
ls | grep dshlib.c | wc -w
EOF

    # Strip all whitespace (spaces, tabs, newlines) from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Expected output with all whitespace removed for easier matching
    expected_output="dshlib.cdsh3>dsh3>cmdloopreturned0"

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


@test "Check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Check ls runs with arguments" {
    run ./dsh <<EOF
ls -l /home
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Exit command returns correct exit status" {
    # Capture the exit status after running the dsh shell with the 'exit' command
    run "./dsh" <<EOF
exit
EOF

    # Output the captured exit status
    echo "Captured exit status: $status"

    # The expected exit status should be 0 when 'exit' is called
    [ "$status" -eq 0 ]
}

@test "Command not found displays correct status" {
    # Run a non-existent command
    run ./dsh <<EOF
nonexistentcommand
EOF

    # Check that the output contains the expected error message
    echo "Captured output: $output"
    echo "Captured exit status: $status"
    
    # Ensure the error message is in stderr
    [[ "$output" =~ "Command not found in PATH" ]]
    
}

@test "cd command changes directory" {
    run ./dsh <<EOF
cd /tmp
EOF
    [ "$status" -eq 0 ]
}

@test "Check for empty command" {
    run ./dsh <<EOF

EOF
    [ "$status" -eq 0 ]
}

@test "check for redirection file creation" {
  # Run the echo command and redirect to a file by passing input via a pipe
  echo 'echo "hello, class" > out.txt' | ./dsh

  # Verify that the file contains the correct output without quotes
  run cat out.txt
  [ "$status" -eq 0 ]
  [ "$output" == "hello, class" ]
}

@test "check for correct redirection file printing" {
  # Create the output file from Test 1 (this ensures the file exists before use)
  echo 'echo "hello, class" > out.txt' | ./dsh

  # Check if the file exists before proceeding
  run ls out.txt
  [ "$status" -eq 0 ]

  # Run the dsh command with input redirection to read from out.txt
  run cat out.txt

  # Verify the output is what we expect (the content from out.txt, without quotes)
  [ "$status" -eq 0 ]
  [ "$output" == "hello, class" ]
}