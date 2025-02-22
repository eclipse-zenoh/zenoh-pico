import os
import signal
import subprocess
import sys
import time

# Specify the directory for the binaries
DIR_EXAMPLES = "build/examples"

NO_LEAK_OUTPUT = "All heap blocks were freed -- no leaks are possible"

VALGRIND_CMD = f"stdbuf -oL -eL valgrind --leak-check=full ./{DIR_EXAMPLES}/"


def failure_mode(fail_cmd):
    test_status = 0
    # Start binary
    print("Start binary")
    z_pub_command = VALGRIND_CMD + fail_cmd
    z_pub_process = subprocess.Popen(
        z_pub_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    # Wait for process to finish
    z_pub_process.wait()
    # Check output
    print("Check output")
    z_pub_output = z_pub_process.stderr.read()
    if NO_LEAK_OUTPUT in z_pub_output:
        print("Failure mode output valid")
    else:
        print("Failure mode output invalid:")
        print(f"Received: \"{z_pub_output}\"")
        test_status = 1
    # Return value
    return test_status


def pub_and_sub(pub_cmd, sub_cmd):
    test_status = 0

    print(f"Start {sub_cmd}")
    # Start z_sub in the background
    z_sub_command = VALGRIND_CMD + sub_cmd

    z_sub_process = subprocess.Popen(
        z_sub_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    # Introduce a delay to ensure z_sub starts
    time.sleep(2)

    print(f"Start {pub_cmd}")
    # Start z_pub
    z_pub_command = VALGRIND_CMD + pub_cmd
    z_pub_process = subprocess.Popen(
        z_pub_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    # Wait for z_pub to finish
    z_pub_process.wait()

    print(f"Stop {sub_cmd}")
    time.sleep(2)
    if z_sub_process.poll() is None:
        # send SIGINT to group
        z_sub_process_gid = os.getpgid(z_sub_process.pid)
        os.killpg(z_sub_process_gid, signal.SIGINT)

    # Wait for z_sub to finish
    z_sub_process.wait()

    print("Check outputs")
    # Check output of z_pub
    z_pub_output = z_pub_process.stderr.read()
    if NO_LEAK_OUTPUT in z_pub_output:
        print(f"{pub_cmd} output valid")
    else:
        print(f"{pub_cmd} output invalid:")
        print(f"Received: \"{z_pub_output}\"")
        test_status = 1

    # Check output of z_sub
    z_sub_output = z_sub_process.stderr.read()
    if NO_LEAK_OUTPUT in z_sub_output:
        print(f"{sub_cmd} output valid")
    else:
        print(f"{sub_cmd} output invalid:")
        print(f"Received: \"{z_sub_output}\"")
        test_status = 1
    # Return value
    return test_status


def query_and_queryable(query_cmd, queryable_cmd):
    test_status = 0
    print(f"Start {queryable_cmd}")
    # Start z_queryable in the background
    z_queryable_command = VALGRIND_CMD + queryable_cmd
    z_queryable_process = subprocess.Popen(
        z_queryable_command,
        shell=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    # Introduce a delay to ensure z_queryable starts
    time.sleep(2)

    print(f"Start {query_cmd}")
    # Start z_query
    z_query_command = VALGRIND_CMD + query_cmd
    z_query_process = subprocess.Popen(
        z_query_command,
        shell=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    # Wait for z_query to finish
    z_query_process.wait()

    print(f"Stop {queryable_cmd}")
    time.sleep(5)
    if z_queryable_process.poll() is None:
        # send SIGINT to group
        z_quaryable_process_gid = os.getpgid(z_queryable_process.pid)
        os.killpg(z_quaryable_process_gid, signal.SIGINT)

    # Wait for z_queryable to finish
    z_queryable_process.wait()

    print("Check outputs")
    # Check output of z_query
    z_query_output = z_query_process.stderr.read()
    if NO_LEAK_OUTPUT in z_query_output:
        print(f"{query_cmd} output valid")
    else:
        print(f"{query_cmd} output invalid:")
        print(f'Received: "{z_query_output}"')
        test_status = 1

    # Check output of z_queryable
    z_queryable_output = z_queryable_process.stderr.read()
    if NO_LEAK_OUTPUT in z_queryable_output:
        print(f"{queryable_cmd} output valid")
    else:
        print(f"{queryable_cmd} output invalid:")
        print(f'Received: "{z_queryable_output}"')
        test_status = 1
    # Return status
    return test_status


if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    EXIT_STATUS = 0

    # Test failure mode
    print("*** Failure mode ***")
    if failure_mode('z_pub -m peer') == 1:
        EXIT_STATUS = 1
    # Test pub and sub examples
    print("*** Pub & sub test ***")
    if pub_and_sub('z_pub -n 1', 'z_sub -n 1') == 1:
        EXIT_STATUS = 1
    print("*** Pub & sub attachment test ***")
    if pub_and_sub('z_pub_attachment -n 1', 'z_sub_attachment -n 1') == 1:
        EXIT_STATUS = 1
    print("*** Pub & sub listener test ***")
    if pub_and_sub('z_pub -n 1 -a', 'z_sub -n 1') == 1:
        EXIT_STATUS = 1
    # Test query and queryable examples
    print("*** Query & queryable test ***")
    if query_and_queryable('z_get', 'z_queryable -n 1') == 1:
        EXIT_STATUS = 1
    print("*** Query & queryable attachment test ***")
    if query_and_queryable('z_get_attachment -v Something', 'z_queryable_attachment -n 1') == 1:
        EXIT_STATUS = 1
    # Test querier and queryable examples
    print("*** Querier & queryable test ***")
    if query_and_queryable('z_querier -n 1', 'z_queryable -n 1') == 1:
        EXIT_STATUS = 1
    # Test liveliness query
    print("*** Get liveliness test ***")
    if query_and_queryable('z_get_liveliness', 'z_liveliness -t 3') == 1:
        EXIT_STATUS = 1
    # Test liveliness subscriber
    print("*** Liveliness subscriber test ***")
    if query_and_queryable('z_liveliness -t 1', 'z_sub_liveliness -h -n 1') == 1:
        EXIT_STATUS = 1
    # Exit
    sys.exit(EXIT_STATUS)
