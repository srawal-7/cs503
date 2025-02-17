#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF
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