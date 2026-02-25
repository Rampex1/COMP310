import subprocess
import sys
from collections import Counter
from pathlib import Path

GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"

HERE = Path(__file__).resolve().parent
SRC = (HERE / "../src").resolve()
SHELL = SRC / "mysh"

# MT tests: output ordering is non-deterministic due to thread scheduling.
# They are compared by line-frequency counts rather than exact diff.
MT_TESTS = {"T_MT1.txt", "T_MT2.txt", "T_MT3.txt"}

# (input, [result1, result2, ...])
TESTS = [
    ("T_FCFS.txt",      ["T_FCFS_result.txt"]),
    ("T_FCFS2.txt",     ["T_FCFS2_result.txt"]),
    ("T_FCFS3.txt",     ["T_FCFS3_result.txt"]),
    ("T_FCFS4.txt",     ["T_FCFS4_result.txt"]),
    ("T_SJF.txt",       ["T_SJF_result.txt"]),
    ("T_SJF2.txt",      ["T_SJF2_result.txt"]),
    ("T_SJF3.txt",      ["T_SJF3_result.txt"]),
    ("T_SJF4.txt",      ["T_SJF4_result.txt"]),
    ("T_RR.txt",        ["T_RR_result.txt"]),
    ("T_RR2.txt",       ["T_RR2_result.txt"]),
    ("T_RR3.txt",       ["T_RR3_result.txt"]),
    ("T_RR4.txt",       ["T_RR4_result.txt"]),
    ("T_RR30.txt",      ["T_RR30_result.txt"]),
    ("T_RR30_2.txt",    ["T_RR30_2_result.txt"]),
    ("T_AGING.txt",     ["T_AGING_result.txt", "T_AGING_result2.txt"]),
    ("T_AGING2.txt",    ["T_AGING2_result.txt"]),
    ("T_AGING3.txt",    ["T_AGING3_result.txt"]),
    ("T_AGING4.txt",    ["T_AGING4_result.txt", "T_AGING4_result2.txt"]),
    ("T_MT1.txt",       ["T_MT1_result.txt"]),
    ("T_MT2.txt",       ["T_MT2_result.txt"]),
    ("T_MT3.txt",       ["T_MT3_result.txt"]),
    ("T_background.txt",["T_background_result.txt"]),
    ("T_source.txt",    ["T_source_result.txt"]),
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


def freq_compare(out_file, expected_file):
    """Compare output by line-frequency counts (order-insensitive)."""
    actual = Counter(l.strip() for l in out_file.read_text().splitlines() if l.strip())
    expected = Counter(l.strip() for l in expected_file.read_text().splitlines() if l.strip())
    if actual == expected:
        return True, ""
    diff_lines = []
    for line in sorted(actual.keys() | expected.keys()):
        a, e = actual[line], expected[line]
        if a != e:
            diff_lines.append(f"  '{line}': got {a}, expected {e}")
    return False, "\n".join(diff_lines)


def run_test(input_file, expected_files, flexible=False):
    out_file = input_file.with_suffix(".out")

    with input_file.open("r") as fin, out_file.open("w") as fout:
        subprocess.run([str(SHELL)], stdin=fin, stdout=fout, cwd=HERE)

    if flexible:
        # MT tests: ordering is non-deterministic, compare by line frequency
        for expected_file in expected_files:
            if not expected_file.exists():
                continue
            ok, msg = freq_compare(out_file, expected_file)
            if ok:
                return True, ""
        _, msg = freq_compare(out_file, expected_files[0])
        return False, msg

    # pass if output matches any of the valid expected results
    for expected_file in expected_files:
        if not expected_file.exists():
            continue
        diff = sh(f"diff -Bw {out_file} {expected_file}")
        if diff.returncode == 0:
            return True, ""

    # all failed — return diff against first result for display
    diff = sh(f"diff -Bw {out_file} {expected_files[0]}")
    return False, diff.stdout


# build
print(f"{GREEN}Rebuilding shell...{RESET}")
build = sh("make clean && make mysh", cwd=SRC)
if build.returncode != 0:
    print(build.stderr)
    sys.exit(1)

# run tests
FAILED = False
for input_name, result_names in TESTS:
    input_path = HERE / input_name
    expected_paths = [HERE / r for r in result_names]

    print(f"Running {input_name}...")

    if not input_path.exists():
        print(f"  MISSING INPUT: {input_name}")
        FAILED = True
        continue

    ok, diff_output = run_test(input_path, expected_paths, flexible=input_name in MT_TESTS)

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
