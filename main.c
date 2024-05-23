#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    uint8_t v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, vA, vB, vC, vD, vE, vF, sound_delay, sound_time, stack_ptr;
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

    // Reads the ROM into memory with the appropriate offset (0x200, see Cowgod's Chip 8 reference)
    read_bytes = fread(cpu_ptr->memory + 0x200, 1, file_size, file_ptr);
    // Some classic print debugging to check how many bytes were read into the CPU memory
    printf("A total of %d bytes were read.\n", read_bytes);
    fclose(file_ptr);

    return 0;
}

int main(void) {

    chip8_cpu CPU = init();
    char *path = "Cave.ch8";
    load_rom(&CPU, path);
    
    for (int i = 0; i < 4096; i++) {
        printf("%d", CPU.memory[i]);
    }
    
    printf("\n");
    
    return 0;
}