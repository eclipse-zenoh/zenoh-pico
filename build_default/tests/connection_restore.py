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
SUBSCRIBER_RECEIVE_MESSAGES = ["[Subscriber] Received"]
ROUTER_ERROR_MESSAGE = "ERROR"
WRITE_FILTER_OFF_MESSAGE = "write filter state: 1"
WRITE_FILTER_ACTIVE_MESSAGE = "write filter state: 0"
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


def test_pub_sub_survive_router_restart(router_command, pub_command, sub_command, timeout):
    """
    Start router, start subscriber and publisher, verify samples flow.
    Restart router, wait for clients to reconnect, verify samples still flow.
    """
    print(f"Pub/Sub flow across router restart (timeout={timeout})")

    router_output = []
    pub_output    = []
    sub_output    = []
    router_ps = []
    pub_ps    = []
    sub_ps    = []

    try:
        # Router up
        run_background(router_command, router_output, router_ps)
        time.sleep(ROUTER_INIT_TIMEOUT_S)

        # Start subscriber first so it can declare interest
        run_background(sub_command, sub_output, sub_ps)
        wait_connect(sub_output)   # session up from sub side
        sub_output.clear()

        # Start publisher
        run_background(pub_command, pub_output, pub_ps)
        wait_connect(pub_output)   # session up from pub side
        pub_output.clear()

        # Confirm baseline delivery before restart
        print("Verifying baseline delivery...")
        if not wait_messages(sub_output, SUBSCRIBER_RECEIVE_MESSAGES):
            raise Exception("Baseline: subscriber did not receive any sample.")
        sub_output.clear()

        # Restart router
        print("Restarting router...")
        terminate_processes(router_ps)
        time.sleep(timeout)

        # Both client logs should eventually show disconnect; don't hard-fail if only one shows it
        try:
            wait_disconnect(pub_output)
            pub_output.clear()
        except Exception:
            pass
        try:
            wait_disconnect(sub_output)
            sub_output.clear()
        except Exception:
            pass

        run_background(router_command, router_output, router_ps)
        time.sleep(ROUTER_INIT_TIMEOUT_S)

        # Reconnect
        wait_reconnect(pub_output)
        #pub_output.clear()
        wait_reconnect(sub_output)
        sub_output.clear()

        # Verify delivery after restart
        print("Verifying delivery after router restart...")
        if not wait_messages(sub_output, SUBSCRIBER_RECEIVE_MESSAGES):
            raise Exception("After restart: subscriber did not receive any sample.")
        if not wait_messages(pub_output, WRITE_FILTER_OFF_MESSAGE):
            raise Exception("After restart: write filter state not updated to off.")

        check_router_errors(router_output)
        print("Pub/Sub flow across router restart: PASSED")
    finally:
        terminate_processes(sub_ps + pub_ps + router_ps)


def test_pub_before_restart_then_new_sub(router_command, pub_command, sub_command, timeout):
    """
    Start router and publisher; restart router; wait for publisher to reconnect;
    then start a new subscriber and verify it receives samples.
    This reproduces the writer-side filtering / missing interest issue.
    """
    print(f"Pub before restart, new Sub after (timeout={timeout})")

    router_output = []
    pub_output    = []
    sub_output    = []
    router_ps = []
    pub_ps    = []
    sub_ps    = []

    try:
        # Router up
        run_background(router_command, router_output, router_ps)
        time.sleep(ROUTER_INIT_TIMEOUT_S)

        # Start publisher first
        run_background(pub_command, pub_output, pub_ps)
        wait_connect(pub_output)
        pub_output.clear()

        # Restart router
        print("Restarting router...")
        terminate_processes(router_ps)
        time.sleep(timeout)

        # Publisher should notice disconnect; don't fail if log line format differs
        try:
            wait_disconnect(pub_output)
            pub_output.clear()
        except Exception:
            pass

        run_background(router_command, router_output, router_ps)
        time.sleep(ROUTER_INIT_TIMEOUT_S)

        # Wait for publisher to reconnect
        wait_reconnect(pub_output)
        pub_output.clear()

        # Now start NEW subscriber
        run_background(sub_command, sub_output, sub_ps)
        wait_connect(sub_output)
        sub_output.clear()

        # Critical assertion: subscriber should receive samples
        print("Waiting for subscriber to receive samples...")
        if not wait_messages(sub_output, SUBSCRIBER_RECEIVE_MESSAGES):
            # Print some context for debugging
            print("=== Publisher (last lines) ===")
            [print(l) for l in pub_output[-50:]]
            print("=== Subscriber (last lines) ===")
            [print(l) for l in sub_output[-50:]]
            print("=== Router (last lines) ===")
            [print(l) for l in router_output[-50:]]
            raise Exception(
                "New subscriber did not receive samples after router restart while publisher "
                "existed before. This matches the issue where publisher stays in writer-side "
                "filtering and never resends interest."
            )
        if not wait_messages(pub_output, WRITE_FILTER_OFF_MESSAGE):
            raise Exception("After restart: write filter state not updated to off.")

        check_router_errors(router_output)
        print("Pub before restart, new Sub after: PASSED")
    finally:
        terminate_processes(sub_ps + pub_ps + router_ps)


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

    # Existing z_pub <-> z_sub communication survives a router restart
    test_pub_sub_survive_router_restart(router_command,
                                        ACTIVE_CLIENT_COMMAND,
                                        PASSIVE_CLIENT_COMMAND,
                                        8)

    # After a router restart, a new z_sub can receive samples from a pre-restart z_pub
    test_pub_before_restart_then_new_sub(router_command,
                                         ACTIVE_CLIENT_COMMAND,
                                         PASSIVE_CLIENT_COMMAND,
                                         8)

    test_liveliness_drop(router_command, LIVELINESS_CLIENT_COMMAND, LIVELINESS_SUB_CLIENT_COMMAND)


if __name__ == "__main__":
    main()
