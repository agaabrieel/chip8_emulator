#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    uint8_t V[16], delay_timer, sound_timer, stack_ptr;
    uint16_t I, program_counter, stack[16];
} cpu_registers;

typedef struct
{
    uint8_t memory[4096], display_buffer[64][32], key_input[16];
    cpu_registers registers;
} chip8_cpu;

chip8_cpu init(void) {    
    chip8_cpu CPU;
    return CPU;
}

int load_rom(chip8_cpu *cpu_ptr, const char *path) {
    // Creates file pointer
    FILE *file_ptr = fopen(path, "r");
    int file_size, read_bytes;

    // Ensures the stream pointer is not null
    if (file_ptr == NULL) {
        printf("Falha ao abrir o arquivo, seu viado!");
        return -1;
    }

    // Moves file pointer to the file end to get file size
    if (fseek(file_ptr, 0, SEEK_END) == 0) {
        file_size = ftell(file_ptr);
        if (file_size == -1) {
            fclose(file_ptr);
            return -1;
        }
    }

    // Moves file pointer back to the file beggining
    if (fseek(file_ptr, 0, SEEK_SET) != 0) {
        fclose(file_ptr);
        return -1;
    }

    // Reads the ROM into memory with the appropriate offset (0x200, see Cowgod's Chip 8 technical reference)
    read_bytes = fread(cpu_ptr->memory + 0x200, sizeof(uint8_t), file_size, file_ptr);
    // Some classic print debugging to check how many bytes were read into the CPU memory
    printf("A total of %d bytes were read.\n", read_bytes);
    fclose(file_ptr);

    return 0;
}

void _00E0(chip8_cpu *cpu_ptr) {
    for (int i = 0; i < 64; i++) {
        for (int j = 0; i < 32; j++) {
            cpu_ptr->display_buffer[i][j] = 0;
        }
    }
}

void _00EE(chip8_cpu *cpu_ptr) {
    cpu_ptr->registers.program_counter = cpu_ptr->registers.stack[cpu_ptr->registers.stack_ptr];
    --cpu_ptr->registers.stack_ptr;
}

void _1nnn(chip8_cpu *cpu_ptr, uint16_t lowest_12_bits) {
    cpu_ptr->registers.program_counter = lowest_12_bits;
}

void _2nnn(chip8_cpu *cpu_ptr, uint16_t lowest_12_bits) {
    // raise NonImplementedError
}

void _3xkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] == low_byte) {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _4xkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] != low_byte) {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _5xy0(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_lower_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] == cpu_ptr->registers.V[upper_lower_byte]) {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _6xkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte) {
    cpu_ptr->registers.V[lower_high_byte] = low_byte;
}

void _7xkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte) {
    cpu_ptr->registers.V[lower_high_byte] += low_byte;
}

void _9xy0(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_lower_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] != cpu_ptr->registers.V[upper_lower_byte]) {
        cpu_ptr->registers.program_counter += 2;    
    }
}

void _Annn(chip8_cpu *cpu_ptr, uint16_t lowest_12_bits) {
    cpu_ptr->registers.I = lowest_12_bits;
}

void _Bnnn(chip8_cpu *cpu_ptr, uint16_t lowest_12_bits) {
    cpu_ptr->registers.program_counter = lowest_12_bits + cpu_ptr->registers.V[0x0];
}

void draw_screen(chip8_cpu *cpu_ptr) {
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 32; j++) {
            if (cpu_ptr->display_buffer[i][j] == 1) {
                printf("@");
                continue;
            }
            printf(" ");
        }
        printf("\n");
    }
}

int main(void) {

    chip8_cpu CPU = init();
    char *path = "Cave.ch8";
    load_rom(&CPU, path);
    
    uint16_t opcode, left_nibble, right_nibble, lowest_12_bits;
    uint8_t lower_high_byte, upper_low_byte, low_byte;
    // Main emulator loop
    while (1) {
        // 8-bit shift to the left on the 8-byte sized region of memory pointed at by the pc to put it in the format 0xXX00
        left_nibble = CPU.memory[CPU.registers.program_counter] << 8;
        // Move the index by 1 (8 bytes) to get the second 8-byte sized part of the instruction as 0xXX
        right_nibble = CPU.memory[CPU.registers.program_counter + 1];
        // Perform bitwise OR on both to get the full 16 bytes instruction
        opcode = left_nibble | right_nibble;
        // Process opcode
        switch (opcode & 0xF000) {
            case 0x0000:
                switch (opcode & 0x000F) {
                    case 0x0000:
                        _00E0(&CPU);
                        break;
                    case 0x000E:
                        _00EE(&CPU);
                        break;
                }
            case 0x1000:
                lowest_12_bits = opcode & 0x0FFF;
                _1nnn(&CPU, lowest_12_bits);
                break;
            case 0x2000:
                lowest_12_bits = opcode & 0x0FFF;
                _2nnn(&CPU, lowest_12_bits);
                break;
            case 0x3000:
                // Get the lower 4 bits (nibble/hex digit) of the high byte through a bitwise AND and a bitwise right shift by 1 byte
                lower_high_byte = (opcode & 0x0F00) >> 8;
                // Get the low byte through a bitwise AND
                low_byte = opcode & 0x00FF;
                _3xkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0x4000:
                lower_high_byte = (opcode & 0x0F00) >> 8;
                low_byte = opcode & 0x00FF;
                _4xkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0x5000:
                lower_high_byte = (opcode & 0x0F00) >> 8;
                upper_low_byte = (opcode & 0x00F0) >> 4;
                _5xy0(&CPU, lower_high_byte, upper_low_byte); 
                break;
            case 0x6000:
                lower_high_byte = (opcode & 0x0F00) >> 8;
                low_byte = opcode & 0x00FF;
                _6xkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0x7000:
                lower_high_byte = (opcode & 0x0F00) >> 8;
                low_byte = opcode & 0x00FF;
                _7xkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0x9000:
                lower_high_byte = (opcode & 0x0F00) >> 8;
                upper_low_byte = (opcode & 0x00F0) >> 4;
                _9xy0(&CPU, lower_high_byte, upper_low_byte);
                break;
            case 0xA000:
                lowest_12_bits = opcode & 0x0FFF;
                _Annn(&CPU, lowest_12_bits);
                break;
            case 0xB000:
                lowest_12_bits = opcode & 0x0FFF;
                _Bnnn(&CPU, lowest_12_bits);
                break;            
        }

        CPU.registers.program_counter += 2;

        draw_screen(&CPU);
    }
    
    return 0;
}