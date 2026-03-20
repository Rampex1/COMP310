# A3

Build commands run from `src/`, test commands run from `A3-test-cases/`.

## Running Tests

### Automated

```bash
cd A3-test-cases
python3 run_tests.py
```

### Manual

**tc1, tc2, tc4** (framesize=18)
```bash
make -C src clean && make -C src mysh framesize=18 varmemsize=10
../src/mysh < tc1.txt | diff -Bw - tc1_result.txt
../src/mysh < tc2.txt | diff -Bw - tc2_result.txt
../src/mysh < tc4.txt | diff -Bw - tc4_result.txt
```

**tc3** (framesize=21)
```bash
make -C src clean && make -C src mysh framesize=21 varmemsize=10
../src/mysh < tc3.txt | diff -Bw - tc3_result.txt
```

**tc5** (framesize=6)
```bash
make -C src clean && make -C src mysh framesize=6 varmemsize=10
../src/mysh < tc5.txt | diff -Bw - tc5_result.txt
```
