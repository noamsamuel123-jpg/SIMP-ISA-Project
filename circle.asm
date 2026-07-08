# 1. Initialize center address and radius for testing
.word 0x100 0x8080                   # Center offset: row 128 (0x80), col 128 (0x80) -> 32896
.word 0x101 50                       # Radius: 50 pixels

# 2. Setup and Load Inputs
lw $s0, $zero, $imm1, 0x100, 0       # $s0 = C (Center offset in frame buffer)
lw $s1, $zero, $imm1, 0x101, 0       # $s1 = R (Radius)
mul $s2, $s1, $s1, 0, 0              # $s2 = R^2 (Radius squared for distance check)

# 3. Extract X and Y coordinates of the center (without division)
srl $t0, $s0, $imm1, 8, 0            # $t0 = y_c = C >> 8 (Divide by 256 to get row)
and $t1, $s0, $imm1, 255, 0          # $t1 = x_c = C & 255 (Modulo 256 to get column)

# 4. Calculate Bounding Box Limits
sub $t2, $t0, $s1, 0, 0              # $t2 = y = y_c - R (Start row)
add $t3, $t0, $s1, 0, 0              # $t3 = y_end = y_c + R (End row)
sub $a0, $t1, $s1, 0, 0              # $a0 = x_start = x_c - R (Start column)
add $t5, $t1, $s1, 0, 0              # $t5 = x_end = x_c + R (End column)

# 5. Outer Loop: Iterate over Y (Rows)
OuterLoopY:
bgt $imm1, $t2, $t3, EndCircle, 0    # if (y > y_end) goto EndCircle (Done drawing)
add $t4, $a0, $zero, 0, 0            # $t4 = x = x_start (Reset X for the new row)

# 6. Inner Loop: Iterate over X (Columns)
InnerLoopX:
bgt $imm1, $t4, $t5, NextY, 0        # if (x > x_end) goto NextY (Row finished)

# 7. Calculate Distance from Center: dx^2 + dy^2
sub $a1, $t4, $t1, 0, 0              # $a1 = dx = x - x_c
sub $a2, $t2, $t0, 0, 0              # $a2 = dy = y - y_c
mul $a1, $a1, $a1, 0, 0              # $a1 = dx^2
mul $a2, $a2, $a2, 0, 0              # $a2 = dy^2
add $a3, $a1, $a2, 0, 0              # $a3 = dx^2 + dy^2 (Current pixel distance squared)

# 8. Check if pixel is inside the circle
bgt $imm1, $a3, $s2, NextX, 0        # if (dist^2 > R^2) goto NextX (Pixel is outside, skip)

# 9. Draw Pixel!
# Calculate exact pixel offset in frame buffer: offset = (y * 256) + x
sll $v0, $t2, $imm1, 8, 0            # $v0 = y << 8 (y * 256)
add $v0, $v0, $t4, 0, 0              # $v0 = (y * 256) + x

# Send commands to Hardware I/O Registers
out $v0, $zero, $imm1, 20, 0         # IOReg[20] (monitoraddr) = pixel offset
out $imm1, $zero, $imm2, 255, 21     # IOReg[21] (monitordata) = 255 (White color)
out $imm1, $zero, $imm2, 1, 22       # IOReg[22] (monitorcmd) = 1 (Trigger write)

# 10. Increment X and loop
NextX:
add $t4, $t4, $imm1, 1, 0            # x++
beq $imm1, $zero, $zero, InnerLoopX, 0 # Unconditional jump back to InnerLoopX

# 11. Increment Y and loop
NextY:
add $t2, $t2, $imm1, 1, 0            # y++
beq $imm1, $zero, $zero, OuterLoopY, 0 # Unconditional jump back to OuterLoopY

# 12. Finish execution
EndCircle:
halt $zero, $zero, $zero, 0, 0       # Halt the simulator