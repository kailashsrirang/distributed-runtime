import selectors
import subprocess
import sys


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    coordinator_path, worker_path = sys.argv[1:3]

    # run coordinator asynchronously
    coordinator = subprocess.Popen(
        [coordinator_path],
        stdout=subprocess.PIPE,  # capture stdout to read the "Listening" message
        stderr=subprocess.PIPE,  # capture stderr
        text=True,
    )

    try:
        # Wait for the coordinator listening message before continuing
        with selectors.DefaultSelector() as selector:
            selector.register(coordinator.stdout, selectors.EVENT_READ,)
            require(bool(selector.select(timeout=5)),
                    "Coordinator did not become ready within 5 seconds",)
            ready_line = coordinator.stdout.readline()

        if ready_line.strip() != "Listening on 127.0.0.1:5000":
            stdout, stderr = coordinator.communicate(timeout=5)
            raise RuntimeError(
                "Coordinator failed to start:\n" + ready_line + stdout + stderr)

        # run worker synchronously, capturing output and waiting for it to finish
        worker = subprocess.run(
            [worker_path],
            capture_output=True,
            text=True,
            timeout=5,
        )

        coordinator_stdout, coordinator_stderr = (
            coordinator.communicate(timeout=5)  # collect coordinator's output
        )

        require(worker.returncode == 0, "Worker failed:\n" +
                worker.stdout + worker.stderr)

        require(
            coordinator.returncode == 0,
            "Coordinator failed:\n"
            + coordinator_stdout
            + coordinator_stderr,
        )

        require(
            "Completed 7 tasks covering 26 readings"
            in coordinator_stdout,
            "Coordinator did not verify all tasks:\n"
            + coordinator_stdout,
        )

        require(
            "Verified task 6: count=2" in coordinator_stdout,
            "Final partial block was not verified",
        )

        require(
            "Coordinator closed connection; completed 7 tasks"
            in worker.stdout,
            "Worker did not finish cleanly:\n" + worker.stdout,
        )

        print("Multi-block TCP integration test passed")

    finally:
        if coordinator.poll() is None:
            coordinator.kill()

        coordinator.communicate()


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"Integration test failed: {error}", file=sys.stderr)
        sys.exit(1)
