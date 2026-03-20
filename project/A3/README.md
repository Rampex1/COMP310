Run make commands in src/
Run test commands in A3-test-cases/

# tc1, tc2, tc4 (framesize=18)
  make clean && make mysh framesize=18 varmemsize=10
  ../src/mysh < tc1.txt | diff - tc1_result.txt -Bw
  ../src/mysh < tc2.txt | diff - tc2_result.txt -Bw
  ../src/mysh < tc4.txt | diff - tc4_result.txt -Bw

  # tc3 (framesize=21)
  make clean && make mysh framesize=21 varmemsize=10
  ../src/mysh < tc3.txt | diff - tc3_result.txt -Bw

  # tc5 (framesize=6)
  make clean && make mysh framesize=6 varmemsize=10
  ../src/mysh < tc5.txt | diff - tc5_result.txt -Bw
