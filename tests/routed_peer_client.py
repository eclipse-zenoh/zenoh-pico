"""Test suite for pub (peer) > router > sub (client) scenarios."""
import os
from signal import SIGINT
import subprocess
import sys
import time
import re

# pylint: disable=consider-using-with

# Specify the directory for the binaries
DIR_EXAMPLES = "build/examples"


def pub_and_sub(pub_args, sub_args, pub_first=True):
    """
    Run a publisher and subscriber test.

    Parameters:
        pub_args (str): Arguments passed to the publisher binary.
        sub_args (str): Arguments passed to the subscriber binary.
        pub_first (bool): If True, start publisher first; otherwise start subscriber first.

    Returns:
        int: 0 if the test passes, 1 if it fails.
    """
    test_status = 0

    # Expected z_pub status
    z_pub_expected_status = 0

    # Expected z_sub output & status
    z_sub_expected_status = 0
    z_sub_expected_pattern = re.compile(
        r">> \[Subscriber\] Received \('demo/example/zenoh-pico-pub': '\[.*?\] Pub from Pico!'\)"
    )

    z_pub_command = f"stdbuf -oL -eL ./{DIR_EXAMPLES}/z_pub {pub_args} -n 10"
    z_sub_command = f"stdbuf -oL -eL ./{DIR_EXAMPLES}/z_sub {sub_args} -n 1"

    print("PUB CMD:", z_pub_command)
    print("SUB CMD:", z_sub_command)

    if pub_first:
        print("Start publisher")
        # Start z_pub first
        z_pub_process = subprocess.Popen(
            z_pub_command,
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        # Give publisher time to start
        time.sleep(2)

        print("Start subscriber")
        # Then start z_sub
        z_sub_process = subprocess.Popen(
            z_sub_command,
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            start_new_session=True,
        )
    else:
        print("Start subscriber")
        # Start z_sub first
        z_sub_process = subprocess.Popen(
            z_sub_command,
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            start_new_session=True,
        )

        # Give subscriber time to start
        time.sleep(2)

        print("Start publisher")
        # Then start z_pub
        z_pub_process = subprocess.Popen(
            z_pub_command,
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

    # Wait for z_pub to finish and capture its output
    pub_stdout, pub_stderr = z_pub_process.communicate()

    print("Stop subscriber")
    if z_sub_process.poll() is None:
        # send SIGINT to group (safe because of start_new_session=True)
        z_sub_process_gid = os.getpgid(z_sub_process.pid)
        os.killpg(z_sub_process_gid, SIGINT)

    # Ensure subscriber has exited and capture its output
    sub_stdout, sub_stderr = z_sub_process.communicate()

    print("Check publisher status")
    # Check the exit status of z_pub
    z_pub_status = z_pub_process.returncode
    if z_pub_status == z_pub_expected_status:
        print("z_pub status valid")
    else:
        print(f"z_pub status invalid, expected: {z_pub_expected_status}, received: {z_pub_status}")
        test_status = 1

    print("Check subscriber status & output")
    # Check the exit status of z_sub
    z_sub_status = z_sub_process.returncode
    if z_sub_status == z_sub_expected_status:
        print("z_sub status valid")
    else:
        print(f"z_sub status invalid, expected: {z_sub_expected_status}, received: {z_sub_status}")
        test_status = 1

    # Check the output of z_sub
    if z_sub_expected_pattern.search(sub_stdout):
        print("z_sub output valid")
    else:
        print("z_sub output invalid:")
        test_status = 1

    # If anything failed, dump stdout/stderr for both processes
    if test_status == 1:
        print("==== z_pub STDOUT ====")
        print(pub_stdout or "")
        print("==== z_pub STDERR ====")
        print(pub_stderr or "")

        print("==== z_sub STDOUT ====")
        print(sub_stdout or "")
        print("==== z_sub STDERR ====")
        print(sub_stderr or "")

    # Return value
    return test_status

def test_tcp_unicast(locator, pub_first=True):
    """Run TCP unicast pub/sub test."""
    print(f"*** TCP Unicast Test (pub_first={pub_first}) ***")
    pub_args = f"-m peer -e {locator}"
    sub_args = f"-m client -e {locator}"

    return pub_and_sub(pub_args, sub_args, pub_first)

def test_udp_multicast(tcp_locator, udp_locator, pub_first=True):
    """Run UDP multicast pub/sub test."""
    print(f"*** UDP Multicast Test (pub_first={pub_first}) ***")
    pub_args = f"-m peer -l {udp_locator}"
    sub_args = f"-m client -e {tcp_locator}"

    return pub_and_sub(pub_args, sub_args, pub_first)

if __name__ == "__main__":
    EXIT_STATUS = 0

    TCP_UNICAST_LOCATOR = None # pylint: disable=invalid-name
    UDP_MULTICAST_LOCATOR = None # pylint: disable=invalid-name

    args = sys.argv[1:]

    for arg in args:
        if arg.startswith("tcp/"):
            TCP_UNICAST_LOCATOR = arg
        elif arg.startswith("udp/"):
            UDP_MULTICAST_LOCATOR = arg

    print("TCP unicast locator:", TCP_UNICAST_LOCATOR)
    print("UDP multicast locator:", UDP_MULTICAST_LOCATOR)

    if TCP_UNICAST_LOCATOR is not None:
        if test_tcp_unicast(TCP_UNICAST_LOCATOR, True) == 1:
            EXIT_STATUS = 1
        if test_tcp_unicast(TCP_UNICAST_LOCATOR, False) == 1:
            EXIT_STATUS = 1
    else:
        print("No TCP unicast locator provided, skipping test_tcp_unicast.")

    if TCP_UNICAST_LOCATOR is not None and UDP_MULTICAST_LOCATOR is not None:
        if test_udp_multicast(TCP_UNICAST_LOCATOR, UDP_MULTICAST_LOCATOR, True) == 1:
            EXIT_STATUS = 1
        if test_udp_multicast(TCP_UNICAST_LOCATOR, UDP_MULTICAST_LOCATOR, False) == 1:
            EXIT_STATUS = 1
    else:
        print("No TCP unicast or UDP multicast locators provided, skipping test_udp_multicast.")

    sys.exit(EXIT_STATUS)
