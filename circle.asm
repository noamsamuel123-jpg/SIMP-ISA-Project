# circle.asm
# Draws a filled white circle on the 256x256 monitor.
# Inputs:
# MEM[0x100]: Center offset ( (y_c << 8) | x_c )
# MEM[0x101]: Radius (R)

# 1. Initialize center address and radius for testing
.word 0x100 0x8080                   # Center: row 128, col 128
.word 0x101 50                       # Radius: 50 pixels

# Register Mapping:
# $s0 (R11) = Center row (y_c)
# $s1 (R12) = Center col (x_c)
# $v0 (R3)  = Radius squared (R^2)
# $t0 (R8)  = Current y (loop counter)
# $t1 (R9)  = Current x (loop counter)
# $t2 (R10) = y_end
# $a0 (R4)  = x_start
# $a1 (R5)  = x_end
# $a2 (R6)  = Temp / Distance check
# $a3 (R7)  = Temp / Pixel Address

# 2. Setup and Load Inputs
lw $a2, $zero, $imm1, 0x100, 0       # Load center offset
srl $s0, $a2, $imm1, 8, 0            # $s0 = y_c = offset >> 8
and $s1, $a2, $imm1, 255, 0          # $s1 = x_c = offset & 0xFF

lw $a2, $zero, $imm1, 0x101, 0       # Load Radius (R)
mul $v0, $a2, $a2, 0, 0              # $v0 = R^2

# 3. Calculate Bounding Box
sub $t0, $s0, $a2, 0, 0              # $t0 = y_start = y_c - R
add $t2, $s0, $a2, 0, 0              # $t2 = y_end = y_c + R
sub $a0, $s1, $a2, 0, 0              # $a0 = x_start = x_c - R
add $a1, $s1, $a2, 0, 0              # $a1 = x_end = x_c + R

# 4. Outer Loop: Iterate over Y (Rows)
OuterLoopY:
bgt $imm1, $t0, $t2, EndCircle, 0    # if (y > y_end) goto End
add $t1, $a0, $zero, 0, 0            # x = x_start

# 5. Inner Loop: Iterate over X (Columns)
InnerLoopX:
bgt $imm1, $t1, $a1, NextY, 0        # if (x > x_end) goto NextY

# 6. Calculate Distance: (x-x_c)^2 + (y-y_c)^2
sub $a2, $t1, $s1, 0, 0              # dx = x - x_c
mul $a2, $a2, $a2, 0, 0              # dx^2
sub $a3, $t0, $s0, 0, 0              # dy = y - y_c
mul $a3, $a3, $a3, 0, 0              # dy^2
add $a2, $a2, $a3, 0, 0              # dist^2 = dx^2 + dy^2

# 7. Check if inside
bgt $imm1, $a2, $v0, NextX, 0        # if (dist^2 > R^2) continue

# 8. Draw Pixel
# Address = (y << 8) | x
sll $a3, $t0, $imm1, 8, 0            # y << 8
add $a3, $a3, $t1, 0, 0              # (y << 8) + x
out $a3, $zero, $imm1, 20, 0         # monitoraddr
add $a2, $zero, $imm1, 255, 0        # White color
out $a2, $zero, $imm1, 21, 0         # monitordata
add $a2, $zero, $imm1, 1, 0          # Command: write
out $a2, $zero, $imm1, 22, 0         # monitorcmd

# 9. Increment and Loop
NextX:
add $t1, $t1, $imm1, 1, 0            # x++
beq $imm1, $zero, $zero, InnerLoopX, 0

NextY:
add $t0, $t0, $imm1, 1, 0            # y++
beq $imm1, $zero, $zero, OuterLoopY, 0

EndCircle:
halt $zero, $zero, $zero, 0, 0
