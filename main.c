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

int main(void) {

    chip8_cpu CPU = init();
    char *path = "Cave.ch8";
    load_rom(&CPU, path);
    
    uint16_t opcode;
    // Main emulator loop
    while (1) {
        // 8-bit shift to the left on the 8-byte sized region of memory pointed at by the pc to put it in the format 0xXX00
        uint16_t left_nibble = CPU.memory[CPU.registers.program_counter] << 8;
        // Move the index by 1 (8 bytes) to get the second 8-byte sized part of the instruction as 0xXX
        uint8_t right_nibble = CPU.memory[CPU.registers.program_counter + 1];
        // Perform bitwise OR on both to get the full 16 bytes instruction
        opcode = left_nibble | right_nibble;
        // Process opcode
        switch (opcode & 0xF000) {
            case 0x0000:
                switch (opcode & 0x000F) {
                    case 0x0000:
                        // Clear screen instruction...
                        CPU.registers.program_counter += 2;
                        break;
                    case 0x000E:
                        // Returns from subroutine...
                        CPU.registers.program_counter += 2;
                        break;
                }
        }

        CPU.registers.program_counter += 2;
    }
    
    return 0;
}