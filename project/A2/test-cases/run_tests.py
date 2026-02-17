import subprocess
import sys
from pathlib import Path

GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"

# paths for testing
HERE = Path(__file__).resolve().parent
SRC = (HERE / "../src").resolve()
SHELL = SRC / "mysh"

INPUTS = HERE / "inputs"
EXPECTED = HERE / "expected"

TESTS = [
    ("echo.txt", "echo_result.txt"),
    ("ls.txt", "ls_result.txt"),
    ("mkdir.txt", "mkdir_result.txt"),
    ("set.txt", "set_result.txt"),
    ("source.txt", "source_result.txt"),
    ("oneline.txt", "oneline_result.txt"),
    ("oneline2.txt", "oneline2_result.txt"),
    ("badcommand.txt", "badcommand_result.txt"),
    ("prompt.txt", "prompt_result.txt"),
    ("run.txt", "run_result.txt"),
]

FAILED = False


# helpers
def sh(cmd, cwd=None):
    return subprocess.run(
        cmd,
        shell=True,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def run_test(input_file, expected_file):
    out_file = input_file.with_suffix(".out")

    with input_file.open("r") as fin, out_file.open("w") as fout:
        subprocess.run([str(SHELL)], stdin=fin, stdout=fout)

    diff = sh(f"diff -Bw {out_file} {expected_file}")
    return diff.returncode == 0, diff.stdout


# build
print(f"{GREEN}Rebuilding shell...{RESET}")
build = sh("make clean && make mysh", cwd=SRC)

if build.returncode != 0:
    print(build.stderr)
    sys.exit(1)

# run tests
for name, expected_name in TESTS:
    input_path = INPUTS / name
    expected_path = EXPECTED / expected_name

    print(f"Running {name}...")

    if not input_path.exists() or not expected_path.exists():
        print("  MISSING FILE")
        FAILED = True
        continue

    ok, diff_output = run_test(input_path, expected_path)

    if ok:
        print(f"  {GREEN}PASS{RESET}")
    else:
        print(f"  {RED}FAIL{RESET}")
        print(diff_output)
        FAILED = True


if FAILED:
    print("\nSome tests failed.")
    sys.exit(1)
else:
    print("\nAll tests passed.")
