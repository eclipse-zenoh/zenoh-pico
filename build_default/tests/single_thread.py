import subprocess
import signal
import sys
import time

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
Putting Data ('demo/example/zenoh-pico-pub': '[   4] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   5] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   6] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   7] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   8] Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': '[   9] Pub from Pico!')...'''

    # Expected z_sub output
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

    print("Start subscriber")
    # Start z_sub in the background
    z_sub_command = f"./{DIR_EXAMPLES}/z_sub_st -n 10"
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
    z_pub_command = f"./{DIR_EXAMPLES}/z_pub_st -n 10"
    z_pub_process = subprocess.Popen(z_pub_command,
                                    shell=True,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    text=True)

    # Wait for z_pub to finish
    z_pub_process.wait()
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
    EXIT_STATUS = 0

    # Test pub and sub examples
    if pub_and_sub() == 1:
        EXIT_STATUS = 1
    # Exit
    sys.exit(EXIT_STATUS)
