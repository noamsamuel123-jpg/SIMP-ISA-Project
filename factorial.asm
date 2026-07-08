# 1. Initialize n at address 0x100 (testing with n = 5)
.word 0x100 5

# 2. Main Program Setup
# Initialize Stack Pointer ($sp) to address 0x800
add $sp, $zero, $imm2, 0, 0x800

# Load n from memory address 0x100 into $a0
lw $a0, $zero, $imm1, 0x100, 0

# 3. Call the Factorial function
jal $ra, $imm1, $zero, Factorial, 0

# 4. Store the result (from $v0) to memory address 0x101
sw $v0, $zero, $imm1, 0x101, 0

# Halt execution
halt $zero, $zero, $zero, 0, 0

# Factorial Function
# Input: $a0 = n
# Output: $v0 = n!

Factorial:
# 5. Base Case: if (n == 0) goto BaseCase
beq $imm1, $a0, $zero, BaseCase, 0

# 6. Recursive Step: Allocate space on stack and save registers
sub $sp, $sp, $imm1, 2, 0            # Allocate 2 words on stack
sw $ra, $sp, $imm1, 1, 0             # Save Return Address ($ra) at MEM[$sp + 1]
sw $a0, $sp, $zero, 0, 0             # Save argument n ($a0) at MEM[$sp]

# 7. Calculate n - 1 and make recursive call
sub $a0, $a0, $imm1, 1, 0            # $a0 = n - 1
jal $ra, $imm1, $zero, Factorial, 0  # Recursive call: factorial(n - 1)

# 8. Restore registers from stack after returning
lw $a0, $sp, $zero, 0, 0             # Restore argument n into $a0
lw $ra, $sp, $imm1, 1, 0             # Restore Return Address into $ra
add $sp, $sp, $imm1, 2, 0            # Deallocate 2 words from stack

# 9. Multiply n * factorial(n - 1)
mul $v0, $a0, $v0, 0, 0              # $v0 = n * $v0 (result)

# Jump to return logic
beq $imm1, $zero, $zero, Return, 0   # Unconditional jump to Return

# 10. Base Case Logic
BaseCase:
add $v0, $zero, $imm1, 1, 0          # $v0 = 1 (since 0! = 1)

# 11. Return to caller
Return:
beq $ra, $zero, $zero, 0, 0          # pc = $ra (Return to the calling function)