Assignment 3 - Demand Paging & Scheduling
COMP 310 / ECSE 427 - Winter 2026

Authors:
  David Zhou          - 261135446
  William Kiem Lafond - 261176778

Starter Code:
  This submission builds on top of our Assignment 2 implementation,
  which itself was based on the original starter code provided by the OS team.

Implementation Notes:
  - Build commands must be run from the `src/` directory.
  - Test commands must be run from the `A3-test-cases/` directory.
  - Different test cases require different frame sizes, so recompilation is needed between tests.

Running Tests:

  Automated:
    cd test-cases
    python3 run_tests.py

  Manual:

    tc1, tc2, tc4 (framesize=18)
      make -C src clean && make -C src mysh framesize=18 varmemsize=10
      ../src/mysh < tc1.txt | diff -Bw - tc1_result.txt
      ../src/mysh < tc2.txt | diff -Bw - tc2_result.txt
      ../src/mysh < tc4.txt | diff -Bw - tc4_result.txt

    tc3 (framesize=21)
      make -C src clean && make -C src mysh framesize=21 varmemsize=10
      ../src/mysh < tc3.txt | diff -Bw - tc3_result.txt

    tc5 (framesize=6)
      make -C src clean && make -C src mysh framesize=6 varmemsize=10
