import subprocess
import time
import sys
import os
import threading

ROUTER_INIT_TIMEOUT_S = 3
WAIT_MESSAGE_TIMEOUT_S = 15
DISCONNECT_MESSAGES = ["Closing session because it has expired", "Send keep alive failed"]
CONNECT_MESSAGES = ["Z_OPEN(Ack)"]
LIVELINESS_TOKEN_ALIVE_MESSAGES = ["[LivelinessSubscriber] New alive token"]
LIVELINESS_TOKEN_DROP_MESSAGES = ["[LivelinessSubscriber] Dropped token"]
ROUTER_ERROR_MESSAGE = "ERROR"
ZENOH_PORT = "7447"

ROUTER_ARGS = ['-l', f'tcp/0.0.0.0:{ZENOH_PORT}', '--no-multicast-scouting']
STDBUF_CMD = ["stdbuf", "-o0"]

DIR_EXAMPLES = "build/examples"
ACTIVE_CLIENT_COMMAND = STDBUF_CMD + [f'{DIR_EXAMPLES}/z_pub', '-e', f'tcp/127.0.0.1:{ZENOH_PORT}']
PASSIVE_CLIENT_COMMAND = STDBUF_CMD + [f'{DIR_EXAMPLES}/z_sub', '-e', f'tcp/127.0.0.1:{ZENOH_PORT}']

LIVELINESS_CLIENT_COMMAND = STDBUF_CMD + [f'{DIR_EXAMPLES}/z_liveliness', '-e', f'tcp/127.0.0.1:{ZENOH_PORT}']
LIVELINESS_SUB_CLIENT_COMMAND = STDBUF_CMD + [f'{DIR_EXAMPLES}/z_sub_liveliness', '-h', '-e', f'tcp/127.0.0.1:{ZENOH_PORT}']

LIBASAN_PATH = subprocess.run(["gcc", "-print-file-name=libasan.so"], stdout=subprocess.PIPE, text=True, check=True).stdout.strip()


def run_process(command, output_collector, process_list):
    env = os.environ.copy()
    env["RUST_LOG"] = "trace"
    if LIBASAN_PATH:
        env["LD_PRELOAD"] = LIBASAN_PATH

    print(f"Run {command}")
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
    process_list.append(process)
    for line in iter(process.stdout.readline, ''):
        print(f"-- [{process.pid}]:", line.strip())
        output_collector.append(line.strip())
    process.stdout.close()
    process.wait()


def run_background(command, output_collector, process_list):
    thread = threading.Thread(target=run_process, args=(command, output_collector, process_list))
    thread.start()


def terminate_processes(process_list):
    for process in process_list:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
    process_list.clear()


def block_connection():
    subprocess.run(["iptables", "-A", "INPUT", "-p", "tcp", "--dport", ZENOH_PORT, "-j", "DROP"], check=True)
    subprocess.run(["iptables", "-A", "OUTPUT", "-p", "tcp", "--sport", ZENOH_PORT, "-j", "DROP"], check=True)


def unblock_connection():
    subprocess.run(["iptables", "-D", "INPUT", "-p", "tcp", "--dport", ZENOH_PORT, "-j", "DROP"], check=False)
    subprocess.run(["iptables", "-D", "OUTPUT", "-p", "tcp", "--sport", ZENOH_PORT, "-j", "DROP"], check=False)


def wait_messages(client_output, messages):
    start_time = time.time()
    while time.time() - start_time < WAIT_MESSAGE_TIMEOUT_S:
        if any(message in line for line in client_output for message in messages):
            return True
        time.sleep(1)
    return False


def wait_connect(client_output):
    if wait_messages(client_output, CONNECT_MESSAGES):
        print("Initial connection successful.")
    else:
        raise Exception("Connection failed.")


def wait_reconnect(client_output):
    if wait_messages(client_output, CONNECT_MESSAGES):
        print("Connection restored successfully.")
    else:
        raise Exception("Failed to restore connection.")


def wait_disconnect(client_output):
    if wait_messages(client_output, DISCONNECT_MESSAGES):
        print("Connection lost successfully.")
    else:
        raise Exception("Failed to block connection.")


def check_router_errors(router_output):
    for line in router_output:
        if ROUTER_ERROR_MESSAGE in line:
            print(line)
            raise Exception("Router have an error.")


def test_connection_drop(router_command, client_command, timeout):
    print(f"Drop test {client_command} for timeout {timeout}")
    router_output = []
    client_output = []
    process_list = []
    blocked = False
    try:
        run_background(router_command, router_output, process_list)
        time.sleep(ROUTER_INIT_TIMEOUT_S)

        run_background(client_command, client_output, process_list)

        # Two iterations because there was an error on the second reconnection
        for _ in range(2):
            wait_connect(client_output)
            client_output.clear()

            print("Blocking connection...")
            block_connection()
            blocked = True

            time.sleep(timeout)

            wait_disconnect(client_output)
            client_output.clear()

            print("Unblocking connection...")
            unblock_connection()
            blocked = False

            wait_reconnect(client_output)

        check_router_errors(router_output)

        print(f"Drop test {client_command} for timeout {timeout} passed")
    finally:
        if blocked:
            unblock_connection()
        terminate_processes(process_list)


def test_router_restart(router_command, client_command, timeout):
    print(f"Restart test {client_command} for timeout {timeout}")
    router_output = []
    client_output = []
    router_process_list = []
    client_process_list = []
    try:
        run_background(router_command, router_output, router_process_list)
        time.sleep(ROUTER_INIT_TIMEOUT_S)

        run_background(client_command, client_output, client_process_list)

        wait_connect(client_output)
        client_output.clear()

        print("Stop router...")
        terminate_processes(router_process_list)

        time.sleep(timeout)

        wait_disconnect(client_output)
        client_output.clear()

        print("Start router...")
        run_background(router_command, router_output, router_process_list)
        time.sleep(ROUTER_INIT_TIMEOUT_S)

        wait_reconnect(client_output)

        check_router_errors(router_output)

        print(f"Restart test {client_command} for timeout {timeout} passed")
    finally:
        terminate_processes(client_process_list + router_process_list)


def test_liveliness_drop(router_command, liveliness_command, liveliness_sub_command):
    print(f"Liveliness drop test")
    router_output = []
    dummy_output = []
    client_output = []
    process_list = []
    blocked = False
    try:
        run_background(router_command, router_output, process_list)
        time.sleep(ROUTER_INIT_TIMEOUT_S)

        run_background(liveliness_sub_command, client_output, process_list)
        run_background(liveliness_command, dummy_output, process_list)

        if wait_messages(client_output, LIVELINESS_TOKEN_ALIVE_MESSAGES):
            print("Liveliness token alive")
        else:
            raise Exception("Failed to get liveliness token alive.")
        client_output.clear()

        print("Blocking connection...")
        block_connection()
        blocked = True

        time.sleep(15)

        if wait_messages(client_output, LIVELINESS_TOKEN_DROP_MESSAGES):
            print("Liveliness token dropped")
        else:
            raise Exception("Failed to get liveliness token drop.")
        client_output.clear()

        print("Unblocking connection...")
        unblock_connection()
        blocked = False

        if wait_messages(client_output, LIVELINESS_TOKEN_ALIVE_MESSAGES):
            print("Liveliness token alive")
        else:
            raise Exception("Failed to get liveliness token alive.")
        client_output.clear()

        check_router_errors(router_output)

        print(f"Liveliness drop test passed")
    finally:
        if blocked:
            unblock_connection()
        terminate_processes(process_list)


def main():
    if len(sys.argv) != 2:
        print("Usage: sudo python3 ./connection_restore.py /path/to/zenohd")
        sys.exit(1)

    router_command = STDBUF_CMD + [sys.argv[1]] + ROUTER_ARGS

    # timeout less than sesson timeout
    test_connection_drop(router_command, ACTIVE_CLIENT_COMMAND, 15)
    test_connection_drop(router_command, PASSIVE_CLIENT_COMMAND, 15)

    # timeout more than sesson timeout
    test_connection_drop(router_command, ACTIVE_CLIENT_COMMAND, 15)
    test_connection_drop(router_command, PASSIVE_CLIENT_COMMAND, 15)

    # timeout less than sesson timeout
    test_router_restart(router_command, ACTIVE_CLIENT_COMMAND, 8)
    test_router_restart(router_command, PASSIVE_CLIENT_COMMAND, 8)

    # timeout more than sesson timeout
    test_router_restart(router_command, ACTIVE_CLIENT_COMMAND, 15)
    test_router_restart(router_command, PASSIVE_CLIENT_COMMAND, 15)

    test_liveliness_drop(router_command, LIVELINESS_CLIENT_COMMAND, LIVELINESS_SUB_CLIENT_COMMAND)


if __name__ == "__main__":
    main()
