# 1. Setup Memory Buffers
add $s0, $zero, $imm1, 0x100, 0      # $s0 = SumBuffer address (0x100)
add $s1, $zero, $imm1, 0x200, 0      # $s1 = ReadBuffer address (0x200)

# 2. Read Sector 0 directly into SumBuffer
WaitDisk1:
in $t0, $zero, $imm1, 17, 0          # Read diskstatus (IO Reg 17) into $t0
bne $imm1, $t0, $zero, WaitDisk1, 0  # If disk is busy ($t0 != 0), keep waiting

# Setup DMA to read Sector 0
out $zero, $zero, $imm1, 15, 0       # IOReg[15] (disksector) = 0
out $s0, $zero, $imm1, 16, 0         # IOReg[16] (diskbuffer) = 0x100
out $imm1, $zero, $imm2, 1, 14       # IOReg[14] (diskcmd) = 1 (Read command)

WaitDisk2:
in $t0, $zero, $imm1, 17, 0          # Read diskstatus
bne $imm1, $t0, $zero, WaitDisk2, 0  # Wait for Sector 0 read to finish
out $zero, $zero, $imm1, 4, 0        # Clear irq1status (IO Reg 4) to 0

# 3. Loop over Sectors 1 to 7
add $s2, $zero, $imm1, 1, 0          # $s2 = s = 1 (Sector counter)

SectorLoop:
bge $imm1, $s2, $imm2, WriteSector8, 8 # If s >= 8, exit SectorLoop

WaitDisk3:
in $t0, $zero, $imm1, 17, 0          # Read diskstatus
bne $imm1, $t0, $zero, WaitDisk3, 0  # Wait until disk is free

# Setup DMA to read Sector 's'
out $s2, $zero, $imm1, 15, 0         # IOReg[15] (disksector) = s ($s2)
out $s1, $zero, $imm1, 16, 0         # IOReg[16] (diskbuffer) = 0x200
out $imm1, $zero, $imm2, 1, 14       # IOReg[14] (diskcmd) = 1 (Read command)

WaitDisk4:
in $t0, $zero, $imm1, 17, 0          # Read diskstatus
bne $imm1, $t0, $zero, WaitDisk4, 0  # Wait for DMA to load Sector 's' to memory
out $zero, $zero, $imm1, 4, 0        # Clear irq1status (IO Reg 4) to 0

# 4. Add ReadBuffer to SumBuffer (128 words)
add $t1, $zero, $zero, 0, 0          # $t1 = i = 0 (Word counter)

WordLoop:
bge $imm1, $t1, $imm2, EndWordLoop, 128 # If i >= 128, end WordLoop

lw $a0, $s0, $t1, 0, 0               # $a0 = SumBuffer[i]
lw $a1, $s1, $t1, 0, 0               # $a1 = ReadBuffer[i]
add $a0, $a0, $a1, 0, 0              # $a0 = SumBuffer[i] + ReadBuffer[i]
sw $a0, $s0, $t1, 0, 0               # SumBuffer[i] = $a0

add $t1, $t1, $imm1, 1, 0            # i++
beq $imm1, $zero, $zero, WordLoop, 0 # Unconditional jump to WordLoop

EndWordLoop:
add $s2, $s2, $imm1, 1, 0            # s++ (Increment Sector counter)
beq $imm1, $zero, $zero, SectorLoop, 0 # Jump back to next sector

# 5. Write SumBuffer to Sector 8
WriteSector8:
WaitDisk5:
in $t0, $zero, $imm1, 17, 0          # Read diskstatus
bne $imm1, $t0, $zero, WaitDisk5, 0  # Wait until disk is free

# Setup DMA to write Sector 8
out $imm1, $zero, $imm2, 8, 15       # IOReg[15] (disksector) = 8
out $s0, $zero, $imm1, 16, 0         # IOReg[16] (diskbuffer) = 0x100
out $imm1, $zero, $imm2, 2, 14       # IOReg[14] (diskcmd) = 2 (Write command)

WaitDisk6:
in $t0, $zero, $imm1, 17, 0          # Read diskstatus
bne $imm1, $t0, $zero, WaitDisk6, 0  # Wait for DMA to write Sector 8 to disk
out $zero, $zero, $imm1, 4, 0        # Clear irq1status (IO Reg 4) to 0

# 6. Finish execution
halt $zero, $zero, $zero, 0, 0       # Halt the simulator