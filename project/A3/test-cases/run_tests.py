import re
import subprocess
import sys
from pathlib import Path

GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"

HERE = Path(__file__).resolve().parent
SRC = (HERE / "../src").resolve()
SHELL = SRC / "mysh"

# (input, result)
TESTS = [
    ("tc1.txt", "tc1_result.txt"),
    ("tc2.txt", "tc2_result.txt"),
    ("tc3.txt", "tc3_result.txt"),
    ("tc4.txt", "tc4_result.txt"),
    ("tc5.txt", "tc5_result.txt"),
]


def sh(cmd, cwd=None):
    return subprocess.run(
        cmd,
        shell=True,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def parse_mem_sizes(result_file):
    """Parse Frame Store Size and Variable Store Size from the first line of a result file."""
    first_line = result_file.read_text().splitlines()[0]
    m = re.search(r"Frame Store Size = (\d+).*Variable Store Size = (\d+)", first_line)
    if not m:
        print(f"  Could not parse memory sizes from: {first_line}")
        sys.exit(1)
    return int(m.group(1)), int(m.group(2))


def build(framesize, varmemsize):
    print(f"{GREEN}Building with framesize={framesize} varmemsize={varmemsize}...{RESET}")
    result = sh(f"make clean && make mysh framesize={framesize} varmemsize={varmemsize}", cwd=SRC)
    if result.returncode != 0:
        print(result.stderr)
        sys.exit(1)


def run_test(input_file, expected_file):
    out_file = input_file.with_suffix(".out")
    with input_file.open("r") as fin, out_file.open("w") as fout:
        subprocess.run([str(SHELL)], stdin=fin, stdout=fout, cwd=HERE)

    diff = sh(f"diff -Bw {out_file} {expected_file}")
    return diff.returncode == 0, diff.stdout


# Run tests
FAILED = False
for input_name, result_name in TESTS:
    input_path = HERE / input_name
    expected_path = HERE / result_name

    print(f"Running {input_name}...")

    if not input_path.exists():
        print(f"  {RED}MISSING INPUT: {input_name}{RESET}")
        FAILED = True
        continue

    if not expected_path.exists():
        print(f"  {RED}MISSING RESULT: {result_name}{RESET}")
        FAILED = True
        continue

    framesize, varmemsize = parse_mem_sizes(expected_path)
    build(framesize, varmemsize)

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
