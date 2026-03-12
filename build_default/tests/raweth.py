import argparse
import os
from signal import SIGINT
import subprocess
import sys
import time

# Specify the directory for the binaries
DIR_EXAMPLES = "build/examples"

def pub_and_sub(args):
    print("*** Pub & sub test ***")
    test_status = 0

    # Expected z_pub output & status
    if args.reth == 1:
        z_pub_expected_status = 0
        z_pub_expected_output = '''Opening session...
Waiting for joins...
Declaring publisher for 'demo/example/zenoh-pico-pub'...
Press CTRL-C to quit...
Putting Data ('demo/example/zenoh-pico-pub': '[   0] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   1] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   2] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   3] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   4] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   5] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   6] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   7] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   8] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   9] Pub from Pico!')...'''
    else :
        z_pub_expected_status = 255
        z_pub_expected_output = '''Opening session...
Unable to open session!'''

    # Expected z_sub output & status
    if args.reth == 1:
        z_sub_expected_status = 0
        z_sub_expected_output = '''Opening session...
Declaring Subscriber on 'demo/example/**'...
Press CTRL-C to quit...
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   0] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   1] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   2] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   3] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   4] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   5] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   6] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   7] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   8] Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': '[   9] Pub from Pico!')'''
    else :
        z_sub_expected_status = 255
        z_sub_expected_output = '''Opening session...
Unable to open session!'''

    print("Start subscriber")
    # Start z_sub in the background
    z_sub_command = f"sudo stdbuf -oL -eL ./{DIR_EXAMPLES}/z_sub -n 10 -m \"peer\" -l \
                    \"reth/30:03:8c:c8:00:a2#iface=lo;whitelist=30:03:8c:c8:00:a1,\"s"  

    z_sub_process = subprocess.Popen(z_sub_command,
                                    shell=True,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    text=True)

    # Introduce a delay to ensure z_sub starts
    time.sleep(2)

    print("Start publisher")
    # Start z_pub
    z_pub_command = f"sudo stdbuf -oL -eL ./{DIR_EXAMPLES}/z_pub -n 10 -m \"peer\" -l \
                    \"reth/30:03:8c:c8:00:a1#iface=lo;whitelist=30:03:8c:c8:00:a2,\"s"
    z_pub_process = subprocess.Popen(z_pub_command,
                                    shell=True,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    text=True)

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
    z_sub_output = z_sub_process.stdout.read()
    if z_sub_expected_output in z_sub_output:
        print("z_sub output valid")
    else:
        print("z_sub output invalid:")
        print(f"Expected: \"{z_sub_expected_output}\"")
        print(f"Received: \"{z_sub_output}\"")
        test_status = 1
    # Return value
    return test_status

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="This script runs zenoh-pico examples"
                                     " and checks them according to the given configuration")
    parser.add_argument("--reth", type=int, choices=[0, 1],
                        help="Z_FEATURE_RAWETH_TRANSPORT (0 or 1)")

    EXIT_STATUS = 0
    prog_args = parser.parse_args()
    print(f"Args value, reth:{prog_args.reth}")

    # Test pub and sub examples
    if pub_and_sub(prog_args) == 1:
        EXIT_STATUS = 1
    # Exit
    sys.exit(EXIT_STATUS)
