import signal
import unittest

SUCCESS = 0
FAILURE = 1
DIBBLER_CLIENT = "dibbler-client"


class MockProcess:
    def __init__(self, name=DIBBLER_CLIENT, exits_on_term=True):
        self.name = name
        self.running = True
        self.exits_on_term = exits_on_term
        self.signals = []

    def send(self, sig):
        if not self.running:
            return FAILURE
        self.signals.append(sig)
        if sig == signal.SIGKILL or (sig == signal.SIGTERM and self.exits_on_term):
            self.running = False
        return SUCCESS


class MockRuntime:
    def __init__(self, process=None):
        self.process = process

    def is_process_running(self, process_name):
        return self.process is not None and self.process.running and self.process.name == process_name

    def wait_for_process_exit(self, process_name):
        return not self.is_process_running(process_name)


def stop_dhcpv6_client(runtime):
    if runtime.process is None or runtime.process.send(signal.SIGTERM) != SUCCESS:
        return FAILURE
    if runtime.wait_for_process_exit(DIBBLER_CLIENT):
        return SUCCESS
    if runtime.process.send(signal.SIGKILL) != SUCCESS:
        return FAILURE
    return SUCCESS if runtime.wait_for_process_exit(DIBBLER_CLIENT) else FAILURE


class DibblerStopTests(unittest.TestCase):
    def test_graceful_stop_does_not_force_kill(self):
        process = MockProcess()
        self.assertEqual(stop_dhcpv6_client(MockRuntime(process)), SUCCESS)
        self.assertEqual(process.signals, [signal.SIGTERM])

    def test_force_kills_dibbler_after_term_timeout(self):
        process = MockProcess(exits_on_term=False)
        self.assertEqual(stop_dhcpv6_client(MockRuntime(process)), SUCCESS)
        self.assertEqual(process.signals, [signal.SIGTERM, signal.SIGKILL])

    def test_does_not_kill_reused_pid_with_different_name(self):
        process = MockProcess(exits_on_term=False)
        runtime = MockRuntime(process)
        original_wait = runtime.wait_for_process_exit

        def replace_process(process_name):
            process.name = "other-process"
            return original_wait(process_name)

        runtime.wait_for_process_exit = replace_process
        self.assertEqual(stop_dhcpv6_client(runtime), SUCCESS)
        self.assertEqual(process.signals, [signal.SIGTERM])

    def test_missing_process_fails_initial_signal(self):
        self.assertEqual(stop_dhcpv6_client(MockRuntime()), FAILURE)


if __name__ == "__main__":
    unittest.main()
