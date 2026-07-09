#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Sizes
#define MEMORY_SIZE 4096
#define NUM_REGS 16
#define NUM_IO_REGS 23
#define DISK_SIZE (128 * 128)
#define MONITOR_WIDTH 256
#define MONITOR_HEIGHT 256
#define MONITOR_SIZE (MONITOR_WIDTH * MONITOR_HEIGHT)

// data structures for registers
int32_t memory[MEMORY_SIZE] = {0};
int32_t regs[NUM_REGS] = {0};
int32_t io_regs[NUM_IO_REGS] = {0};
int32_t disk[DISK_SIZE] = {0};
uint8_t monitor[MONITOR_SIZE] = {0};
int pc = 0;
int cycle = 0;
int disk_timer = 0;
bool is_in_interrupt = false;

// IRQ2 handling
int *irq2_cycles = NULL;
int irq2_count = 0;
int irq2_idx = 0;

const char *io_reg_names[] = {
    "irq0enable", "irq1enable", "irq2enable", "irq0status", "irq1status", "irq2status",
    "irqhandler", "irqreturn", "clks", "leds", "display7seg", "timerenable",
    "timercurrent", "timermax", "diskcmd", "disksector", "diskbuffer", "diskstatus",
    "reserved", "reserved", "monitoraddr", "monitordata", "monitorcmd"
};

// Helper function for Sign Extension
// Duplicates bit 11 to all upper bits (31:12)
int32_t sign_extend(int32_t imm12) {
    // Check if bit 11 is on (negative number)
    if (imm12 & 0x00000800) {
        return imm12 | 0xFFFFF000;
    }
    return imm12;
}

// Helper function to enforce read-only registers 0, 1, and 2
void enforce_readonly_regs() {
    regs[0] = 0; // register 0 is always zero
    // Registers 1 and 2 (imm1, imm2) are only updated during decode and must not written
}

// Reading the file memin.txt to the memory
void load_memory(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        exit(1);
    }

    int i = 0;
    uint32_t temp_val;
    
    while (fscanf(file, "%x", &temp_val) == 1 && i < MEMORY_SIZE) {
        memory[i] = (int32_t)temp_val;
        i++;
    }

    fclose(file);
}

// Loading diskin.txt
void load_disk(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) return;

    int i = 0;
    uint32_t temp_val;
    while (fscanf(file, "%x", &temp_val) == 1 && i < DISK_SIZE) {
        disk[i] = (int32_t)temp_val;
        i++;
    }
    fclose(file);
}

// Writing diskout.txt
void write_diskout(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) return;

    for (int i = 0; i < DISK_SIZE; i++) {
        fprintf(file, "%08X\n", disk[i]);
    }
    fclose(file);
}

// Writing monitor.txt
void write_monitor_txt(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) return;

    for (int i = 0; i < MONITOR_SIZE; i++) {
        fprintf(file, "%02X\n", monitor[i]);
    }
    fclose(file);
}

// Writing monitor.yuv
void write_monitor_yuv(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) return;

    fwrite(monitor, sizeof(uint8_t), MONITOR_SIZE, file);
    fclose(file);
}

// Loading irq2in.txt
void load_irq2(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return;
    }

    int count = 0;
    int temp;
    while (fscanf(file, "%d", &temp) == 1) {
        count++;
    }
    
    if (count > 0) {
        irq2_cycles = (int *)malloc(count * sizeof(int));
        fseek(file, 0, SEEK_SET);
        int i = 0;
        while (fscanf(file, "%d", &irq2_cycles[i]) == 1) {
            i++;
        }
        irq2_count = count;
    }

    fclose(file);
}


// Function to write the registers to regout.txt at the end of execution
void write_regout(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Could not open %s for writing\n", filename);
        exit(1);
    }

    // starting from 3 for not printing R0, R1, R2
    for (int i = 3; i < NUM_REGS; i++) {
        fprintf(file, "%08X\n", regs[i]);
    }

    fclose(file);
}

// Function to write the memory to memout.txt at the end of execution
void write_memout(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Could not open %s for writing\n", filename);
        exit(1);
    }

    for (int i = 0; i < MEMORY_SIZE; i++) {
        fprintf(file, "%08X\n", memory[i]);
    }

    fclose(file);
}


// Function to write the current state to trace.txt
void write_trace(FILE *trace_file, int32_t current_pc, uint32_t inst, int current_cycle) {
    // Write cycle, PC (3 digits), and instruction
    fprintf(trace_file, "%08X %03X %08X", current_cycle, current_pc & 0xFFF, inst);
    
    for (int i = 0; i < NUM_REGS; i++) {
        fprintf(trace_file, " %08X", regs[i]);
    }
    fprintf(trace_file, "\n");
}


int main(int argc, char *argv[]) {
    // checking if all arguments were supplied
    if (argc < 14) {
        printf("Usage: sim.exe memin.txt diskin.txt irq2in.txt memout.txt regout.txt trace.txt hwregtrace.txt cycles.txt leds.txt display7seg.txt diskout.txt monitor.txt monitor.yuv\n");
        return 1;
    }
    
    // Load memory
    load_memory(argv[1]);
    load_disk(argv[2]);
    load_irq2(argv[3]);
    
    // open files
    FILE *trace_file = fopen(argv[6], "w");
    FILE *hwregtrace_file = fopen(argv[7], "w");
    FILE *leds_file = fopen(argv[9], "w");
    FILE *display7seg_file = fopen(argv[10], "w");
    
    // Check if files opened successfully
    if (trace_file == NULL || hwregtrace_file == NULL || leds_file == NULL || display7seg_file == NULL) {
        printf("Error: Could not open one or more output files for writing.\n");
        if (trace_file) fclose(trace_file);
        if (hwregtrace_file) fclose(hwregtrace_file);
        if (leds_file) fclose(leds_file);
        if (display7seg_file) fclose(display7seg_file);
        return 1;
    }
    
    bool halt_reached = false;

    // Main simulator loop - runs until halt instruction
    while (!halt_reached) {
        // 1. Interrupt check logic (sampling at instruction boundary)
        int irq = ((io_regs[0] & io_regs[3]) | (io_regs[1] & io_regs[4]) | (io_regs[2] & io_regs[5]));
        if (irq && !is_in_interrupt) {
            is_in_interrupt = true;
            io_regs[7] = pc; // irqreturn
            pc = io_regs[6]; // irqhandler
        }

        // 2. Fetch
        uint32_t inst = memory[pc];

        // 3. Decode
        uint8_t opcode = (inst >> 24) & 0xFF;
        uint8_t rd     = (inst >> 20) & 0xF;
        uint8_t rs     = (inst >> 16) & 0xF;
        uint8_t rt     = (inst >> 12) & 0xF;
        int32_t imm12  = inst & 0xFFF;

        // Initialize first immediate with sign extension
        regs[1] = sign_extend(imm12);

        // Check if the instruction requires the second immediate (register 2)
        int use_imm2 = (rd == 2 || rs == 2 || rt == 2);
        if (use_imm2) {
            regs[2] = memory[pc + 1];
        } else {
            regs[2] = 0; // Requirement: Write 8 zeros if not used
        }

        // 4. Calculate trace cycle (second cycle if 2-cycle instruction)
        int trace_cycle = cycle + (use_imm2 ? 1 : 0);

        // 5. Write trace before execution
        write_trace(trace_file, pc, inst, trace_cycle);

        // 6. Pre-calculate next_pc
        int32_t next_pc = pc + (use_imm2 ? 2 : 1);
        
        // 7. Advance time (once or twice)
        for (int i = 0; i < (use_imm2 ? 2 : 1); i++) {
            // Handle IRQ2 from file
            if (irq2_idx < irq2_count && cycle >= irq2_cycles[irq2_idx]) {
                io_regs[5] = 1; // irq2status
                irq2_idx++;
            }

            cycle++;
            io_regs[8] = cycle; // Update clks IO register

            // Timer logic
            if (io_regs[11] == 1) { // timerenable
                if (io_regs[12] >= io_regs[13]) { // timercurrent >= timermax
                    io_regs[3] = 1; // irq0status
                    io_regs[12] = 0; // timercurrent
                } else {
                    io_regs[12]++; // timercurrent++
                }
            }

            // Disk logic
            if (disk_timer > 0) {
                disk_timer--;
                if (disk_timer == 0) {
                    // DMA transfer
                    int sector_start = io_regs[15] * 128;
                    int mem_start = io_regs[16];
                    if (io_regs[14] == 1) { // read sector
                        for (int j = 0; j < 128; j++) {
                            if (mem_start + j < MEMORY_SIZE)
                                memory[mem_start + j] = disk[sector_start + j];
                        }
                    } else if (io_regs[14] == 2) { // write sector
                        for (int j = 0; j < 128; j++) {
                            if (mem_start + j < MEMORY_SIZE)
                                disk[sector_start + j] = memory[mem_start + j];
                        }
                    }
                    io_regs[14] = 0; // diskcmd
                    io_regs[17] = 0; // diskstatus
                    io_regs[4] = 1;  // irq1status
                }
            }
        }

        // 8. Execute instruction
        switch (opcode) {
            case 0: // add
                regs[rd] = regs[rs] + regs[rt];
                pc = next_pc;
                break;
            case 1: // sub
                regs[rd] = regs[rs] - regs[rt];
                pc = next_pc;
                break;
            case 2: // mul
                regs[rd] = regs[rs] * regs[rt];
                pc = next_pc;
                break;
            case 3: // mac
                regs[rd] = (regs[rs] * regs[rt]) + regs[rd];
                pc = next_pc;
                break;
            case 4: // and
                regs[rd] = regs[rs] & regs[rt];
                pc = next_pc;
                break;
            case 5: // or
                regs[rd] = regs[rs] | regs[rt];
                pc = next_pc;
                break;
            case 6: // xor
                regs[rd] = regs[rs] ^ regs[rt];
                pc = next_pc;
                break;
            case 7: // sll
                regs[rd] = regs[rs] << regs[rt];
                pc = next_pc;
                break;
            case 8: // sra
                regs[rd] = regs[rs] >> regs[rt];
                pc = next_pc;
                break;
            case 9: // srl
                regs[rd] = (int32_t)((uint32_t)regs[rs] >> regs[rt]);
                pc = next_pc;
                break;
            case 10: // beq
                pc = (regs[rs] == regs[rt]) ? regs[rd] : next_pc;
                break;
            case 11: // bne
                pc = (regs[rs] != regs[rt]) ? regs[rd] : next_pc;
                break;
            case 12: // blt
                pc = (regs[rs] < regs[rt]) ? regs[rd] : next_pc;
                break;
            case 13: // bgt
                pc = (regs[rs] > regs[rt]) ? regs[rd] : next_pc;
                break;
            case 14: // ble
                pc = (regs[rs] <= regs[rt]) ? regs[rd] : next_pc;
                break;
            case 15: // bge
                pc = (regs[rs] >= regs[rt]) ? regs[rd] : next_pc;
                break;
            case 16: // jal
                regs[rd] = next_pc;
                pc = regs[rs];
                break;
            case 17: // lw
                regs[rd] = memory[regs[rs] + regs[rt]];
                pc = next_pc;
                break;
            case 18: // sw
                memory[regs[rs] + regs[rt]] = regs[rd];
                pc = next_pc;
                break;
            case 19: // reti
                pc = io_regs[7]; // irqreturn
                is_in_interrupt = false;
                break;
            case 20: // in
            {
                int io_idx = regs[rs] + regs[rt];
                if (io_idx >= 0 && io_idx < NUM_IO_REGS) {
                    // Special case for monitorcmd which always returns 0
                    if (io_idx == 22) {
                        regs[rd] = 0;
                    } else {
                        regs[rd] = io_regs[io_idx];
                    }
                    fprintf(hwregtrace_file, "%08X READ %s %08X\n", trace_cycle, io_reg_names[io_idx], regs[rd]);
                }
                pc = next_pc;
                break;
            }
            case 21: // out
            {
                int io_idx = regs[rs] + regs[rt];
                if (io_idx >= 0 && io_idx < NUM_IO_REGS) {
                    int32_t old_val = io_regs[io_idx];
                    io_regs[io_idx] = regs[rd];
                    fprintf(hwregtrace_file, "%08X WRITE %s %08X\n", trace_cycle, io_reg_names[io_idx], io_regs[io_idx]);
                    
                    if (io_idx == 9 && old_val != io_regs[9]) fprintf(leds_file, "%08X %08X\n", trace_cycle, io_regs[9]);
                    else if (io_idx == 10 && old_val != io_regs[10]) fprintf(display7seg_file, "%08X %08X\n", trace_cycle, io_regs[10]);
                    else if (io_idx == 14 && io_regs[17] == 0) { // diskcmd and disk is free
                        io_regs[17] = 1; // diskstatus = busy
                        disk_timer = 1024;
                    }
                    else if (io_idx == 22 && io_regs[22] == 1) { // monitorcmd = write pixel
                        int addr = io_regs[20];
                        if (addr >= 0 && addr < MONITOR_SIZE) {
                            monitor[addr] = (uint8_t)(io_regs[21] & 0xFF);
                        }
                        io_regs[22] = 0; // Reset command
                    }
                }
                pc = next_pc;
                break;
            }
            case 22: // halt
                halt_reached = true;
                break;
            default:
                pc = next_pc;
                break;
        }
         pc &= 0xFFF; //Masking the PC for 12 bits
        // Enforce read-only policy on registers 0, 1, 2
        enforce_readonly_regs();
    }
    
    write_memout(argv[4]);
    write_regout(argv[5]);
    write_diskout(argv[11]);
    write_monitor_txt(argv[12]);
    write_monitor_yuv(argv[13]);
    
    // Write total cycles
    FILE *cycles_file = fopen(argv[8], "w");
    if (cycles_file != NULL) {
        fprintf(cycles_file, "%08X\n", cycle);
        fclose(cycles_file);
    }

    fclose(trace_file);
    fclose(hwregtrace_file);
    fclose(leds_file);
    fclose(display7seg_file);

    if (irq2_cycles != NULL) {
        free(irq2_cycles);
    }

    printf("Simulation completed successfully.\n");

    return 0;
}
