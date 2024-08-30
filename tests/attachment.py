import argparse
import os
from signal import SIGINT
import subprocess
import sys
import time
import re

# Specify the directory for the binaries
DIR_EXAMPLES = "build/examples"


def pub_and_sub():
    print("*** Pub & sub test ***")
    test_status = 0

    # Expected z_pub output & status
    z_pub_expected_status = 0
    z_pub_expected_output = '''Opening session...
Declaring publisher for 'demo/example/zenoh-pico-pub'...
Press CTRL-C to quit...
Putting Data ('demo/example/zenoh-pico-pub': '[   0] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   1] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   2] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   3] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   4] Pub from Pico!')...'''

    # Expected z_sub output & status
    z_sub_expected_status = 0
    z_sub_expected_output = '''Opening session...
Declaring Subscriber on 'demo/example/**'...
Press CTRL-C to quit...
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   0] Pub from Pico!')
    with encoding: zenoh/string;utf8
    with timestamp: 
    with attachment:
     0: source, C
     1: index, 0
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   1] Pub from Pico!')
    with encoding: zenoh/string;utf8
    with timestamp: 
    with attachment:
     0: source, C
     1: index, 1
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   2] Pub from Pico!')
    with encoding: zenoh/string;utf8
    with timestamp: 
    with attachment:
     0: source, C
     1: index, 2
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   3] Pub from Pico!')
    with encoding: zenoh/string;utf8
    with timestamp: 
    with attachment:
     0: source, C
     1: index, 3
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   4] Pub from Pico!')
    with encoding: zenoh/string;utf8
    with timestamp: 
    with attachment:
     0: source, C
     1: index, 4'''

    print("Start subscriber")
    # Start z_sub in the background
    z_sub_command = f"stdbuf -oL -eL ./{DIR_EXAMPLES}/z_sub_attachment -n 5"

    z_sub_process = subprocess.Popen(
        z_sub_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )

    # Introduce a delay to ensure z_sub starts
    time.sleep(2)

    print("Start publisher")
    # Start z_pub
    z_pub_command = f"stdbuf -oL -eL ./{DIR_EXAMPLES}/z_pub_attachment -n 5"
    z_pub_process = subprocess.Popen(
        z_pub_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )

    # Wait for z_pub to finish
    z_pub_process.wait()

    print("Stop subscriber")
    time.sleep(2)
    if z_sub_process.poll() is None:
        # send SIGINT to group
        z_sub_process_gid = os.getpgid(z_sub_process.pid)
        os.killpg(z_sub_process_gid, SIGINT)

    # Wait for z_sub to finish
    z_sub_process.wait()

    print("Check publisher status & output")
    # Check the exit status of z_pub
    z_pub_status = z_pub_process.returncode
    if z_pub_status == z_pub_expected_status:
        print("z_pub status valid")
    else:
        print(f"z_pub status invalid, expected: {z_pub_expected_status}, received: {z_pub_status}")
        test_status = 1

    # Check output of z_pub
    z_pub_output = z_pub_process.stdout.read()
    if z_pub_expected_output in z_pub_output:
        print("z_pub output valid")
    else:
        print("z_pub output invalid:")
        print(f"Expected: \"{z_pub_expected_output}\"")
        print(f"Received: \"{z_pub_output}\"")
        test_status = 1

    print("Check subscriber status & output")
    # Check the exit status of z_sub
    z_sub_status = z_sub_process.returncode
    if z_sub_status == z_sub_expected_status:
        print("z_sub status valid")
    else:
        print(f"z_sub status invalid, expected: {z_sub_expected_status}, received: {z_sub_status}")
        test_status = 1

    # Check output of z_sub
    z_sub_output = re.sub(r'    with timestamp: \d+\n', '    with timestamp: \n', z_sub_process.stdout.read())
    if z_sub_expected_output in z_sub_output:
        print("z_sub output valid")
    else:
        print("z_sub output invalid:")
        print(f"Expected: \"{z_sub_expected_output}\"")
        print(f"Received: \"{z_sub_output}\"")
        test_status = 1
    # Return value
    return test_status


def query_and_queryable():
    print("*** Query & queryable test ***")
    test_status = 0

    # Expected z_query output & status

    z_query_expected_status = 0
    z_query_expected_output = """Opening session...
Sending Query 'demo/example/**'...
>> Received ('demo/example/**': 'Queryable from Pico!')
    with encoding: zenoh/string;utf8
    with attachment:
     0: reply_key, reply_value
>> Received query final notification"""

    # Expected z_queryable output & status
    z_queryable_expected_status = 0
    z_queryable_expected_output = """Opening session...
Creating Queryable on 'demo/example/zenoh-pico-queryable'...
Press CTRL-C to quit...
 >> [Queryable handler] Received Query 'demo/example/**'
    with encoding: zenoh/string;utf8
    with attachment:
     0: test_key, test_value"""

    print("Start queryable")
    # Start z_queryable in the background
    z_queryable_command = f"stdbuf -oL -eL ./{DIR_EXAMPLES}/z_queryable_attachment -n 1"
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

    print("Start query")
    # Start z_query
    z_query_command = f"stdbuf -oL -eL ./{DIR_EXAMPLES}/z_get_attachment"
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

    print("Stop queryable")
    time.sleep(2)
    if z_queryable_process.poll() is None:
        # send SIGINT to group
        z_quaryable_process_gid = os.getpgid(z_queryable_process.pid)
        os.killpg(z_quaryable_process_gid, SIGINT)

    # Wait for z_queryable to finish
    z_queryable_process.wait()

    print("Check query status & output")
    # Check the exit status of z_query
    z_query_status = z_query_process.returncode
    if z_query_status == z_query_expected_status:
        print("z_query status valid")
    else:
        print(f"z_query status invalid, expected: {z_query_expected_status}," f" received: {z_query_status}")
        test_status = 1

    # Check output of z_query
    z_query_output = z_query_process.stdout.read()
    if z_query_expected_output in z_query_output:
        print("z_query output valid")
    else:
        print("z_query output invalid:")
        print(f'Expected: "{z_query_expected_output}"')
        print(f'Received: "{z_query_output}"')
        test_status = 1

    print("Check queryable status & output")
    # Check the exit status of z_queryable
    z_queryable_status = z_queryable_process.returncode
    if z_queryable_status == z_queryable_expected_status:
        print("z_queryable status valid")
    else:
        print(f"z_queryable status invalid, expected: {z_queryable_expected_status}," f" received: {z_queryable_status}")
        test_status = 1

    # Check output of z_queryable
    z_queryable_output = z_queryable_process.stdout.read()
    if z_queryable_expected_output in z_queryable_output:
        print("z_queryable output valid")
    else:
        print("z_queryable output invalid:")
        print(f'Expected: "{z_queryable_expected_output}"')
        print(f'Received: "{z_queryable_output}"')
        test_status = 1
    # Return status
    return test_status


if __name__ == "__main__":
    EXIT_STATUS = 0

    # Test pub and sub examples
    if pub_and_sub() == 1:
        EXIT_STATUS = 1
    # Test query and queryable examples
    if query_and_queryable() == 1:
        EXIT_STATUS = 1
    # Exit
    sys.exit(EXIT_STATUS)
