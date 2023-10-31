import argparse
import subprocess
import sys
import time

# Specify the directory for the binaries
DIR_EXAMPLES = "build/examples"

def pub_and_sub(args):
    print("*** Pub & sub test ***")
    test_status = 0

    # Expected z_pub output & status
    if args.pub == 1:
        z_pub_expected_status = 0
        z_pub_expected_output = '''Opening session...
Declaring publisher for 'demo/example/zenoh-pico-pub'...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...
Putting Data ('demo/example/zenoh-pico-pub': 'Pub from Pico!')...'''
    else :
        z_pub_expected_status = 254
        z_pub_expected_output = "ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it."

    # Expected z_sub output & status
    if args.sub == 1:
        z_sub_expected_status = 0
        if args.pub == 1:
            z_sub_expected_output = '''Opening session...
Declaring Subscriber on 'demo/example/**'...
Enter 'q' to quit...
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')
>> [Subscriber] Received ('demo/example/zenoh-pico-pub': 'Pub from Pico!')'''
        else:
            z_sub_expected_output = '''Opening session...
Declaring Subscriber on 'demo/example/**'...
Enter 'q' to quit...'''
    else :
        z_sub_expected_status = 254
        z_sub_expected_output = "ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it."
    
    print("Start subscriber")
    # Start z_sub in the background
    z_sub_command = f"./{DIR_EXAMPLES}/z_sub"
    z_sub_process = subprocess.Popen(z_sub_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    # Introduce a delay to ensure z_sub starts
    time.sleep(2)

    print("Start publisher")
    # Start z_pub
    z_pub_command = f"./{DIR_EXAMPLES}/z_pub"
    z_pub_process = subprocess.Popen(z_pub_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    # Wait for z_pub to finish
    z_pub_process.wait()

    print("Stop subscriber")
    if z_pub_process.poll() is None:
        # Send "q" command to z_sub to stop it
        z_sub_process.stdin.write("q\n")
        z_sub_process.stdin.flush()
        time.sleep(1)

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
    if z_pub_output.__contains__(z_pub_expected_output):
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
    if z_sub_output.__contains__(z_sub_expected_output):
        print("z_sub output valid")
    else:
        print("z_sub output invalid:")
        print(f"Expected: \"{z_sub_expected_output}\"")
        print(f"Received: \"{z_sub_output}\"")
        test_status = 1
    # Return value
    return test_status

def query_and_queryable(args):
    print("*** Query & queryable test ***")
    test_status = 0

    # Expected z_query output & status
    if args.query == 1:
        z_query_expected_status = 0
        if args.queryable == 1:
            z_query_expected_output = '''Opening session...
Enter any key to pull data or 'q' to quit...
Sending Query 'demo/example/**'...
>> Received ('demo/example/zenoh-pico-queryable': 'Queryable from Pico!')
>> Received query final notification
Sending Query 'demo/example/**'...
>> Received ('demo/example/zenoh-pico-queryable': 'Queryable from Pico!')
>> Received query final notification
Sending Query 'demo/example/**'...
>> Received ('demo/example/zenoh-pico-queryable': 'Queryable from Pico!')
>> Received query final notification'''
        else:
            z_query_expected_output = '''Opening session...
Enter any key to pull data or 'q' to quit...
Sending Query 'demo/example/**'...
>> Received query final notification
Sending Query 'demo/example/**'...\"
>> Received query final notification
Sending Query 'demo/example/**'...
>> Received query final notification'''
    else :
        z_query_expected_status = 254
        z_query_expected_output = "ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY but this example requires it."

    # Expected z_queryable output & status
    if args.queryable == 1:
        z_queryable_expected_status = 0
        if args.query == 1:
            z_queryable_expected_output = '''Opening session...
Creating Queryable on 'demo/example/zenoh-pico-queryable'...
Enter 'q' to quit...
 >> [Queryable handler] Received Query 'demo/example/**?'
 >> [Queryable handler] Received Query 'demo/example/**?'
 >> [Queryable handler] Received Query 'demo/example/**?'''
        else:
            z_queryable_expected_output = '''Opening session...
Creating Queryable on 'demo/example/zenoh-pico-queryable'...
Enter 'q' to quit...'''
    else :
        z_queryable_expected_status = 254
        z_queryable_expected_output = "ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it."
 
    print("Start queryable")
    # Start z_queryable in the background
    z_queryable_command = f"./{DIR_EXAMPLES}/z_queryable"
    z_queryable_process = subprocess.Popen(z_queryable_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    # Introduce a delay to ensure z_queryable starts
    time.sleep(2)

    print("Start query")
    # Start z_query
    z_query_command = f"./{DIR_EXAMPLES}/z_get"
    z_query_process = subprocess.Popen(z_query_command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    # Introduce a delay to ensure z_query starts
    time.sleep(2)

    print("Send requests")
    if z_query_process.poll() is None:
        z_query_process.stdin.write("\n")
        z_query_process.stdin.flush()
        time.sleep(1)
        z_query_process.stdin.write("\n")
        z_query_process.stdin.flush()
        time.sleep(1)
        z_query_process.stdin.write("\n")
        z_query_process.stdin.flush()
        time.sleep(1)

    print("Stop query")
    if z_query_process.poll() is None:
        z_query_process.stdin.write("q\n")
        z_query_process.stdin.flush()

    # Wait for z_query to finish
    z_query_process.wait()

    print("Stop queryable")
    if z_queryable_process.poll() is None:
        # Send "q" command to z_sub to stop it
        z_queryable_process.stdin.write("q\n")
        z_queryable_process.stdin.flush()

    # Wait for z_queryable to finish
    z_queryable_process.wait()

    print("Check query status & output")
    # Check the exit status of z_query
    z_query_status = z_query_process.returncode
    if z_query_status == z_query_expected_status:
        print("z_query status valid")
    else:
        print(f"z_query status invalid, expected: {z_query_expected_status}, received: {z_query_status}")
        test_status = 1

    # Check output of z_query
    z_query_output = z_query_process.stdout.read()
    if z_query_output.__contains__(z_query_expected_output):
        print("z_query output valid")
    else:
        print("z_query output invalid:")
        print(f"Expected: \"{z_query_expected_output}\"")
        print(f"Received: \"{z_query_output}\"")
        test_status = 1

    print("Check queryable status & output")
    # Check the exit status of z_queryable
    z_queryable_status = z_queryable_process.returncode
    if z_queryable_status == z_queryable_expected_status:
        print("z_queryable status valid")
    else:
        print(f"z_queryable status invalid, expected: {z_queryable_expected_status}, received: {z_queryable_status}")
        test_status = 1

    # Check output of z_queryable
    z_queryable_output = z_queryable_process.stdout.read()
    if z_queryable_output.__contains__(z_queryable_expected_output):
        print("z_queryable output valid")
    else:
        print("z_queryable output invalid:")
        print(f"Expected: \"{z_queryable_expected_output}\"")
        print(f"Received: \"{z_queryable_output}\"")
        test_status = 1
    # Return status
    return test_status

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="This script runs zenoh-pico examples and checks them according to the given configuration")
    parser.add_argument("--pub", type=int, choices=[0, 1], help="Z_FEATURE_PUBLICATION (0 or 1)")
    parser.add_argument("--sub", type=int, choices=[0, 1], help="Z_FEATURE_SUBSCRIPTION (0 or 1)")
    parser.add_argument("--queryable", type=int, choices=[0, 1], help="Z_FEATURE_QUERYABLE (0 or 1)")
    parser.add_argument("--query", type=int, choices=[0, 1], help="Z_FEATURE_QUERY (0 or 1)")

    exit_status = 0
    args = parser.parse_args()
    print(f"Args value, pub:{args.pub}, sub:{args.sub}, queryable:{args.queryable}, query:{args.query}")

    # Test pub and sub examples
    if pub_and_sub(args) == 1:
        exit_status = 1
    # Test query and queryable examples
    if query_and_queryable(args) == 1:
        exit_status = 1
    # Exit
    sys.exit(exit_status)