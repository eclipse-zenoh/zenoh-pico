import subprocess
import sys

# Specify the directory for the binaries
DIR_EXAMPLES = "build/examples"
EXPECTED_STATUS = 255
EXPECTED_OUTPUT = """Opening session...
Unable to open session!"""


def run_binary_test(binary_name, expected_status, expected_output):
    print(f"*** Testing {binary_name} ***")
    command = [f"./{DIR_EXAMPLES}/{binary_name}"]

    process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    process.wait()

    # Check output
    output = process.stdout.read()
    if expected_output in output:
        print(f"{binary_name} output valid")
    else:
        print(f'{binary_name} output invalid:\nExpected: "{expected_output}"\nReceived: "{output}"')
        return False

    # Check errors
    errors = process.stderr.read().strip()
    if errors == "":
        print(f"{binary_name} no error reported")
    else:
        print(f'{binary_name} errors reported:\n"{errors}"')
        return False

    # Check exit status
    status = process.returncode
    if status == expected_status:
        print(f"{binary_name} status valid")
    else:
        print(f"{binary_name} status invalid, expected: {expected_status}, received: {status}")
        return False

    return True


def test_all():
    binaries = ["z_sub", "z_pub", "z_queryable", "z_get", "z_liveliness", "z_get_liveliness", "z_sub_liveliness"]

    all_tests_passed = True
    for binary in binaries:
        if not run_binary_test(binary, EXPECTED_STATUS, EXPECTED_OUTPUT):
            all_tests_passed = False

    return all_tests_passed


if __name__ == "__main__":
    EXIT_STATUS = 0

    # Run all tests
    if not test_all():
        EXIT_STATUS = 1

    # Exit with final status
    sys.exit(EXIT_STATUS)
