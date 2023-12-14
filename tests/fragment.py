import subprocess
import sys
import time

# Specify the directory for the binaries
DIR_TESTS = "build/tests"

def check_output(tx_status, tx_output, rx_status, rx_output):
    test_status = 0

    # Expected tx output & status
    z_tx_expected_status = 0
    z_tx_expected_output = "[tx]: Sending packet on test/zenoh-pico-fragment, len: 10000"
    # Expected rx output & status
    z_rx_expected_status = 0
    z_rx_expected_output = (
        "[rx]: Received packet on test/zenoh-pico-fragment, len: 10000, validity: 1")

    # Check the exit status of tx
    if tx_status == z_tx_expected_status:
        print("z_tx status valid")
    else:
        print(f"z_tx status invalid, expected: {z_tx_expected_status}, received: {tx_status}")
        test_status = 1

    # Check output of tx
    if z_tx_expected_output in tx_output:
        print("z_tx output valid")
    else:
        print("z_tx output invalid:")
        print(f"Expected: \"{z_tx_expected_output}\"")
        print(f"Received: \"{tx_output}\"")
        test_status = 1

    # Check the exit status of z_rx
    if rx_status == z_rx_expected_status:
        print("z_rx status valid")
    else:
        print(f"z_rx status invalid, expected: {z_rx_expected_status}, received: {rx_status}")
        test_status = 1

    # Check output of z_rx
    if z_rx_expected_output in rx_output:
        print("z_rx output valid")
    else:
        print("z_rx output invalid:")
        print(f"Expected: \"{z_rx_expected_output}\"")
        print(f"Received: \"{rx_output}\"")
        test_status = 1
    # Return value
    return test_status

def test_client():
    # Start rx in the background
    print("Start rx client")
    z_rx_command = f"./{DIR_TESTS}/z_test_fragment_rx"
    z_rx_process = subprocess.Popen(z_rx_command,
                                    shell=True,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, text=True)
    # Introduce a delay to ensure rx starts
    time.sleep(2)
    # Start tx
    print("Start tx client")
    z_tx_command = f"./{DIR_TESTS}/z_test_fragment_tx"
    z_tx_process = subprocess.Popen(z_tx_command,
                                    shell=True,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    text=True)
    # Wait for tx to finish
    z_tx_process.wait()
    # Wait for rx to receive
    time.sleep(1)
    print("Stop rx")
    if z_rx_process.poll() is None:
        # Send "q" command to rx to stop it
        z_rx_process.stdin.write("q\n")
        z_rx_process.stdin.flush()
    # Wait for rx to finish
    z_rx_process.wait()
    # Check output
    return check_output(z_tx_process.returncode, z_tx_process.stdout.read(),
                        z_rx_process.returncode, z_rx_process.stdout.read())

def test_peer():
    # Start rx in the background
    print("Start rx peer")
    z_rx_command = f"./{DIR_TESTS}/z_test_fragment_rx 1"
    z_rx_process = subprocess.Popen(z_rx_command,
                                    shell=True,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, text=True)
    # Introduce a delay to ensure rx starts
    time.sleep(2)
    # Start tx
    print("Start tx peer")
    z_tx_command = f"./{DIR_TESTS}/z_test_fragment_tx 1"
    z_tx_process = subprocess.Popen(z_tx_command,
                                    shell=True,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    text=True)
    # Wait for tx to finish
    z_tx_process.wait()
    # Wait for rx to receive
    time.sleep(1)
    print("Stop rx")
    if z_rx_process.poll() is None:
        # Send "q" command to rx to stop it
        z_rx_process.stdin.write("q\n")
        z_rx_process.stdin.flush()
    # Wait for rx to finish
    z_rx_process.wait()
    # Check output
    return check_output(z_tx_process.returncode, z_tx_process.stdout.read(),
                        z_rx_process.returncode, z_rx_process.stdout.read())

if __name__ == "__main__":
    EXIT_STATUS = 0

    # Run tests
    if test_client() == 1:
        EXIT_STATUS = 1
    if test_peer() == 1:
        EXIT_STATUS = 1
    # Exit
    sys.exit(EXIT_STATUS)
