#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 500
#define MAX_LABEL_LENGTH 50
#define MEMORY_SIZE 4096
#define MAX_LABELS 4096

typedef struct {
    char name[MAX_LABEL_LENGTH];
    int address;
} Label;

// Global or local variables for the symbol table
Label label_table[MAX_LABELS];
int label_count = 0;

// An array representing the main memory image before writing to file
unsigned int memory_image[MEMORY_SIZE];

// Functions Declaration:
void execute_pass1(FILE* input_file);
void execute_pass2(FILE* input_file);
int get_opcode(const char* opcode_name);
int get_register(const char* reg_name);
int get_label_address(const char* label_name);
int parse_immediate(const char* token);


//-----------------------------------------------------------

int main(int argc, char* argv[])
{
    // Check command-line arguments
    if (argc != 3) {
        printf("Error: Invalid number of arguments.\n");
        return 1;
    }

    FILE* input_file = NULL;
    FILE* output_file = NULL;

    // Open the input file for reading (argv[1] contains the assembly file name)
    input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        printf("Error: Could not open input file %s\n", argv[1]);
        return 1;
    }

    // Open the output file for writing (argv[2] contains memin.txt)
    output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        printf("Error: Could not create output file %s\n", argv[2]);
        fclose(input_file); // Close the already opened input file to prevent memory leaks
        return 1;
    }

    // Initialize the memory image with zeros
    memset(memory_image, 0, sizeof(memory_image));
    
    // Execute Pass 1: Find all labels from the assembly file, calculate their exact memory addresses, and build the lable table.
    execute_pass1(input_file);

    // Reset file pointer for Pass 2
    rewind(input_file);

    // Execute Pass 2: Translate instructions and write to memory image
    execute_pass2(input_file);


    // Write output and cleanup
    for (int i = 0; i < MEMORY_SIZE; i++) {
        fprintf(output_file, "%08X\n", memory_image[i]);
    }

    fclose(input_file);
    fclose(output_file);

    return 0;
}

//-----------------------------------------------
//Functions Definitions

// Pass 1: Identifies labels and calculates their addresses
void execute_pass1(FILE* input_file) {
    char line[MAX_LINE_LENGTH];
    int current_pc = 0;

    while (fgets(line, sizeof(line), input_file) != NULL) {

        // 1. Ignore comments (#)
        char* comment_ptr = strchr(line, '#');
        if (comment_ptr != NULL) {
            *comment_ptr = '\0';
        }

        // 2. Tokenize the line
        char* token = strtok(line, " \t\r\n,");
        if (token == NULL) {
            continue;
        }

        // 3. Check for label (ends with ':')
        int len = strlen(token);
        if (token[len - 1] == ':') {
            token[len - 1] = '\0'; // Remove colon
            strcpy(label_table[label_count].name, token);
            label_table[label_count].address = current_pc;
            label_count++;

            // Check if there's an instruction on the same line
            token = strtok(NULL, " \t\r\n,");
            if (token == NULL) {
                continue;
            }
        }

        // 4. Handle pseudo-instructions
        if (strcmp(token, ".word") == 0 || strcmp(token, ".array") == 0) {
            continue;
        }

        // 5. Standard instruction
        current_pc++;

        // 6. Check for $imm2 to increment PC again
        while ((token = strtok(NULL, " \t\r\n,")) != NULL) {
            if (strcmp(token, "$imm2") == 0) {
                current_pc++;
                break;
            }
        }
    }
}

// Helper function to map an opcode string to its numeric value
int get_opcode(const char* opcode_name) {
    if (strcmp(opcode_name, "add") == 0) return 0;
    if (strcmp(opcode_name, "sub") == 0) return 1;
    if (strcmp(opcode_name, "mul") == 0) return 2;
    if (strcmp(opcode_name, "mac") == 0) return 3;
    if (strcmp(opcode_name, "and") == 0) return 4;
    if (strcmp(opcode_name, "or") == 0) return 5;
    if (strcmp(opcode_name, "xor") == 0) return 6;
    if (strcmp(opcode_name, "sll") == 0) return 7;
    if (strcmp(opcode_name, "sra") == 0) return 8;
    if (strcmp(opcode_name, "srl") == 0) return 9;
    if (strcmp(opcode_name, "beq") == 0) return 10;
    if (strcmp(opcode_name, "bne") == 0) return 11;
    if (strcmp(opcode_name, "blt") == 0) return 12;
    if (strcmp(opcode_name, "bgt") == 0) return 13;
    if (strcmp(opcode_name, "ble") == 0) return 14;
    if (strcmp(opcode_name, "bge") == 0) return 15;
    if (strcmp(opcode_name, "jal") == 0) return 16;
    if (strcmp(opcode_name, "lw") == 0) return 17;
    if (strcmp(opcode_name, "sw") == 0) return 18;
    if (strcmp(opcode_name, "reti") == 0) return 19;
    if (strcmp(opcode_name, "in") == 0) return 20;
    if (strcmp(opcode_name, "out") == 0) return 21;
    if (strcmp(opcode_name, "halt") == 0) return 22;

    return -1; // Return -1 for invalid opcode
}

// Helper function to map a register string to its numeric value
int get_register(const char* reg_name) {
    if (strcmp(reg_name, "$zero") == 0) return 0;
    if (strcmp(reg_name, "$imm1") == 0) return 1;
    if (strcmp(reg_name, "$imm2") == 0) return 2;
    if (strcmp(reg_name, "$v0") == 0) return 3;
    if (strcmp(reg_name, "$a0") == 0) return 4;
    if (strcmp(reg_name, "$a1") == 0) return 5;
    if (strcmp(reg_name, "$a2") == 0) return 6;
    if (strcmp(reg_name, "$a3") == 0) return 7;
    if (strcmp(reg_name, "$t0") == 0) return 8;
    if (strcmp(reg_name, "$t1") == 0) return 9;
    if (strcmp(reg_name, "$t2") == 0) return 10;
    if (strcmp(reg_name, "$s0") == 0) return 11;
    if (strcmp(reg_name, "$s1") == 0) return 12;
    if (strcmp(reg_name, "$gp") == 0) return 13;
    if (strcmp(reg_name, "$sp") == 0) return 14;
    if (strcmp(reg_name, "$ra") == 0) return 15;

    return -1; // Return -1 for invalid register
}

// Helper function to find a label's address from the symbol table
int get_label_address(const char* label_name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(label_table[i].name, label_name) == 0) {
            return label_table[i].address;
        }
    }
    return -1; // Return -1 if the label is not found
}

// Helper function to parse immediate values (decimal, hex, or labels)
int parse_immediate(const char* token) {
    // 1. Check if it's a hexadecimal number
    if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0) {
        return (int)strtoll(token, NULL, 16);
    }
    // 2. Check if it's a decimal number (starts with a digit or a minus sign)
    if (isdigit(token[0]) || token[0] == '-') {
        return atoi(token);
    }
    // 3. If it's neither, it must be a label
    return get_label_address(token);
}

// Pass 2: Translates assembly instructions to machine code and builds the memory image
void execute_pass2(FILE* input_file) {
    char line[MAX_LINE_LENGTH];
    int current_pc = 0;

    while (fgets(line, sizeof(line), input_file) != NULL) {

        // 1. Ignore comments (#)
        char* comment_ptr = strchr(line, '#');
        if (comment_ptr != NULL) {
            *comment_ptr = '\0';
        }

        // 2. Tokenize the line
        char* token = strtok(line, " \t\r\n,");
        if (token == NULL) {
            continue; // Empty line
        }

        // 3. Skip labels (they were handled in Pass 1)
        int len = strlen(token);
        if (token[len - 1] == ':') {
            token = strtok(NULL, " \t\r\n,"); // Get the next word after the label
            if (token == NULL) {
                continue; // Line only had a label
            }
        }

        // 4. Handle .word pseudo-instruction
        if (strcmp(token, ".word") == 0) {
            char* addr_str = strtok(NULL, " \t\r\n,");
            char* data_str = strtok(NULL, " \t\r\n,");

            if (addr_str != NULL && data_str != NULL) {
                int address = parse_immediate(addr_str);
                int data = parse_immediate(data_str);
                memory_image[address] = data;
            }
            continue;
        }

        // 5. Handle .array pseudo-instruction
        if (strcmp(token, ".array") == 0) {
            char* addr_str = strtok(NULL, " \t\r\n,");

            if (addr_str != NULL) {
                int address = parse_immediate(addr_str);
                char* data_str;

                // Read and assign all values in the array
                while ((data_str = strtok(NULL, " \t\r\n,")) != NULL) {
                    memory_image[address] = parse_immediate(data_str);
                    address++;
                }
            }
            continue;
        }

        // 6. Parse standard instruction parameters
        char* opcode_str = token;
        char* rd_str = strtok(NULL, " \t\r\n,");
        char* rs_str = strtok(NULL, " \t\r\n,");
        char* rt_str = strtok(NULL, " \t\r\n,");
        char* imm1_str = strtok(NULL, " \t\r\n,");
        char* imm2_str = strtok(NULL, " \t\r\n,");

        // Validate that we received all parameters
        if (!rd_str || !rs_str || !rt_str || !imm1_str || !imm2_str) {
            continue;
        }

        // 7. Convert strings to numeric values
        int opcode = get_opcode(opcode_str);
        int rd = get_register(rd_str);
        int rs = get_register(rs_str);
        int rt = get_register(rt_str);
        int imm1 = parse_immediate(imm1_str);
        int imm2 = parse_immediate(imm2_str);

        // Check if $imm2 register is used (register number 2)
        int use_imm2 = (rd == 2 || rs == 2 || rt == 2);

        // 8. Build the 32-bit instruction word
        // Mask imm1 with 0xFFF to keep only the lower 12 bits
        unsigned int instruction = (opcode << 24) | (rd << 20) | (rs << 16) | (rt << 12) | (imm1 & 0xFFF);

        // Store the instruction in the memory image
        memory_image[current_pc] = instruction;
        current_pc++;

        // 9. Store the second immediate if $imm2 is used
        if (use_imm2) {
            memory_image[current_pc] = imm2;
            current_pc++;
        }
    }
}




