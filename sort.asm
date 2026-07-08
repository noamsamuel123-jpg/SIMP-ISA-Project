# 1. Initialize test array at address 0x100
.array 0x100 5, 20, 15, 8, 1, 25, 7, 10, 14, 2, 6, 9, 13, 4, 11, 0

# 2. Setup Registers
add $s0, $zero, $imm1, 0x100, 0      # $s0 = base address of the array (0x100)
add $s1, $zero, $imm1, 16, 0         # $s1 = N (Total number of elements = 16)
add $t0, $s1, $zero, 0, 0            # $t0 = i = 16 (Outer loop counter)

# 3. Outer Loop
OuterLoop:
ble $imm1, $t0, $imm2, EndSort, 1    # if (i <= 1) goto EndSort (Array is sorted)
add $t1, $zero, $zero, 0, 0          # $t1 = j = 0 (Inner loop counter)
sub $t2, $t0, $imm1, 1, 0            # $t2 = i - 1 (Inner loop limit)

# 4. Inner Loop
InnerLoop:
bge $imm1, $t1, $t2, EndInner, 0     # if (j >= i - 1) goto EndInner (Exit inner loop)

# 5. Load A[j] and A[j+1] from memory
add $a2, $s0, $t1, 0, 0              # $a2 = base address + j (Address of A[j])
lw $a0, $a2, $zero, 0, 0             # $a0 = A[j] (Load MEM[$a2 + 0])
lw $a1, $a2, $imm1, 1, 0             # $a1 = A[j+1] (Load MEM[$a2 + 1])

# 6. Compare elements for DESCENDING order
bge $imm1, $a0, $a1, NoSwap, 0       # if (A[j] >= A[j+1]) goto NoSwap (Already correct)

# 7. Swap A[j] and A[j+1] if needed
sw $a1, $a2, $zero, 0, 0             # MEM[$a2] = A[j+1]
sw $a0, $a2, $imm1, 1, 0             # MEM[$a2 + 1] = A[j]

# 8. Continue inner loop
NoSwap:
add $t1, $t1, $imm1, 1, 0            # j++ (Increment inner loop counter)
beq $imm1, $zero, $zero, InnerLoop, 0# Unconditional jump to InnerLoop

# 9. Continue outer loop
EndInner:
sub $t0, $t0, $imm1, 1, 0            # i-- (Decrement outer loop counter)
beq $imm1, $zero, $zero, OuterLoop, 0# Unconditional jump to OuterLoop

# 10. Finish execution
EndSort:
halt $zero, $zero, $zero, 0, 0       # Halt the simulator