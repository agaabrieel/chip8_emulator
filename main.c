#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

typedef struct {
    uint8_t V[16], delay_timer, sound_timer;
    uint16_t I, program_counter, stack[16], stack_ptr;
} cpu_registers;

typedef struct {
    uint8_t memory[4096], key_input[16];
    GLubyte display_buffer[64 * 32 * 3]; // 64 x 32 over 3 color channels
    cpu_registers registers;
} chip8_cpu;

void _00E0(chip8_cpu *cpu_ptr) {
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 32; j++) {
            cpu_ptr->display_buffer[(i * 32 + j) * 3 + 0] = 0; // R
            cpu_ptr->display_buffer[(i * 32 + j) * 3 + 1] = 0; // G
            cpu_ptr->display_buffer[(i * 32 + j) * 3 + 2] = 0; // B
        }
    }
    cpu_ptr->registers.program_counter += 2;
}

void _00EE(chip8_cpu *cpu_ptr) {
    cpu_ptr->registers.stack_ptr -= 1;
    cpu_ptr->registers.program_counter = cpu_ptr->registers.stack[cpu_ptr->registers.stack_ptr];
    cpu_ptr->registers.program_counter += 2;
}

void _1nnn(chip8_cpu *cpu_ptr, uint16_t lowest_12_bits) {
    cpu_ptr->registers.program_counter = lowest_12_bits;
}

void _2nnn(chip8_cpu *cpu_ptr, uint16_t lowest_12_bits) {
    cpu_ptr->registers.stack[cpu_ptr->registers.stack_ptr] = cpu_ptr->registers.program_counter;
    cpu_ptr->registers.stack_ptr += 1;
    cpu_ptr->registers.program_counter = lowest_12_bits;
}

void _3xkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] == low_byte) {
        cpu_ptr->registers.program_counter += 4;
    } else {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _4xkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] != low_byte) {
        cpu_ptr->registers.program_counter += 4;
    } else {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _5xy0(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_lower_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] == cpu_ptr->registers.V[upper_lower_byte]) {
        cpu_ptr->registers.program_counter += 4;
    } else {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _6xkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte)
{
    cpu_ptr->registers.V[lower_high_byte] = low_byte;
    cpu_ptr->registers.program_counter += 2;
}

void _7xkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte)
{
    cpu_ptr->registers.V[lower_high_byte] += low_byte;
    cpu_ptr->registers.program_counter += 2;
}

void _8xy0(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    cpu_ptr->registers.V[lower_high_byte] = cpu_ptr->registers.V[upper_low_byte];
    cpu_ptr->registers.program_counter += 2;
}

void _8xy1(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    cpu_ptr->registers.V[lower_high_byte] = cpu_ptr->registers.V[lower_high_byte] | cpu_ptr->registers.V[upper_low_byte];
    cpu_ptr->registers.program_counter += 2;
}

void _8xy2(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    cpu_ptr->registers.V[lower_high_byte] = cpu_ptr->registers.V[lower_high_byte] & cpu_ptr->registers.V[upper_low_byte];
    cpu_ptr->registers.program_counter += 2;
}

void _8xy3(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    cpu_ptr->registers.V[lower_high_byte] = cpu_ptr->registers.V[lower_high_byte] ^ cpu_ptr->registers.V[upper_low_byte];
    cpu_ptr->registers.program_counter += 2;
}

void _8xy4(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    uint8_t sum = (cpu_ptr->registers.V[lower_high_byte] + cpu_ptr->registers.V[upper_low_byte]) & 0xFF;
    cpu_ptr->registers.V[lower_high_byte] = (cpu_ptr->registers.V[lower_high_byte] + cpu_ptr->registers.V[upper_low_byte]) & 0xFF;
    uint16_t carry = sum & 0xF00;
    if (carry) {
        cpu_ptr->registers.V[16] = 1;
    } else {
        cpu_ptr->registers.V[16] = 0;
    }
    cpu_ptr->registers.program_counter += 2;
}

void _8xy5(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    if ((cpu_ptr->registers.V[lower_high_byte] > cpu_ptr->registers.V[upper_low_byte])) {
        cpu_ptr->registers.V[16] = 1;
    } else {
        cpu_ptr->registers.V[16] = 0;
    }
    cpu_ptr->registers.V[lower_high_byte] = (cpu_ptr->registers.V[lower_high_byte] - cpu_ptr->registers.V[upper_low_byte]) & 0xFF;
    cpu_ptr->registers.program_counter += 2;
}

void _8xy6(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] & 0b1) { 
        cpu_ptr->registers.V[16] = 1;
    }
    cpu_ptr->registers.V[lower_high_byte] = cpu_ptr->registers.V[lower_high_byte] >> 2;
    cpu_ptr->registers.program_counter += 2;
}

void _8xy7(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    if ((cpu_ptr->registers.V[lower_high_byte] < cpu_ptr->registers.V[upper_low_byte])) {
        cpu_ptr->registers.V[16] = 1;
    } else {
        cpu_ptr->registers.V[16] = 0;
    }
    cpu_ptr->registers.V[lower_high_byte] = (cpu_ptr->registers.V[upper_low_byte] - cpu_ptr->registers.V[lower_high_byte]) & 0xFF;
    cpu_ptr->registers.program_counter += 2;
}

void _8xyE(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_low_byte) {
    if (cpu_ptr->registers.V[lower_high_byte] >> 8) {
        cpu_ptr->registers.V[16] = 1;
    } else {
        cpu_ptr->registers.V[16] = 0;
    }
    cpu_ptr->registers.program_counter += 2;
}

void _9xy0(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t upper_lower_byte)
{
    if (cpu_ptr->registers.V[lower_high_byte] != cpu_ptr->registers.V[upper_lower_byte])
    {
        cpu_ptr->registers.program_counter += 4;
    } else {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _Annn(chip8_cpu *cpu_ptr, uint16_t lowest_12_bits)
{
    cpu_ptr->registers.I = lowest_12_bits;
    cpu_ptr->registers.program_counter += 2;
}

void _Bnnn(chip8_cpu *cpu_ptr, uint16_t lowest_12_bits)
{
    cpu_ptr->registers.program_counter = lowest_12_bits + cpu_ptr->registers.V[0x0];
}

void _Cxkk(chip8_cpu *cpu_ptr, uint8_t lower_high_byte, uint8_t low_byte) {
    cpu_ptr->registers.V[lower_high_byte] = (rand() % 256) & low_byte; // Modulo operator to keep the random number in the 8 bit range
    cpu_ptr->registers.program_counter += 2;
}

void _Dxyn() {
    printf("Not implemented.\n");
}

void _Ex9E(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    if (cpu_ptr->key_input[lower_high_byte]) {
        cpu_ptr->registers.program_counter += 4;
    } else {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _ExA1(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    if (!cpu_ptr->key_input[lower_high_byte]) {
        cpu_ptr->registers.program_counter += 4;
    } else {
        cpu_ptr->registers.program_counter += 2;
    }
}

void _Fx07(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    cpu_ptr->registers.V[lower_high_byte] = cpu_ptr->registers.delay_timer;
    cpu_ptr->registers.program_counter += 2;
}

void _Fx0A(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    printf("Not implemented.\n");
    cpu_ptr->registers.program_counter += 2;
}

void _Fx15(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    cpu_ptr->registers.delay_timer = cpu_ptr->registers.V[lower_high_byte];
    cpu_ptr->registers.program_counter += 2;
}

void _Fx18(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    cpu_ptr->registers.sound_timer = cpu_ptr->registers.V[lower_high_byte];
    cpu_ptr->registers.program_counter += 2;
}

void _Fx1E(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    cpu_ptr->registers.I = cpu_ptr->registers.I + cpu_ptr->registers.V[lower_high_byte];
    cpu_ptr->registers.program_counter += 2;
}

void _Fx29(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    cpu_ptr->registers.I = 5*(cpu_ptr->registers.V[lower_high_byte]) & 0xFFF;
    cpu_ptr->registers.program_counter += 2;
}

void _Fx33(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    cpu_ptr->memory[cpu_ptr->registers.I] = lower_high_byte / 100;
    cpu_ptr->memory[cpu_ptr->registers.I + 1] = (lower_high_byte % 100) / 10;
    cpu_ptr->memory[cpu_ptr->registers.I + 2] = lower_high_byte % 10;
    cpu_ptr->registers.program_counter += 2;
}

void _Fx55(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    for (int i = 0; i < 16; i++) {
        cpu_ptr->memory[cpu_ptr->registers.I + i] = cpu_ptr->registers.V[i];
    }
    cpu_ptr->registers.program_counter += 2;
}

void _Fx65(chip8_cpu *cpu_ptr, uint8_t lower_high_byte) {
    for (int i = 0; i < 16; i++) {
        cpu_ptr->registers.V[i] = cpu_ptr->memory[cpu_ptr->registers.I + i];
    }
    cpu_ptr->registers.program_counter += 2;
}

void key_callback(GLFWwindow *window_ptr, int key, int scancode, int action, int mods) {

}

chip8_cpu init(void) {
    chip8_cpu CPU;
    int i, j;

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 32; j++) {
            CPU.display_buffer[(i * 32 + j) * 3 + 0] = 0; // R
            CPU.display_buffer[(i * 32 + j) * 3 + 1] = 0; // G
            CPU.display_buffer[(i * 32 + j) * 3 + 2] = 0; // B
        }
    }

    for (i = 0; i < 4096; i++) {
        CPU.memory[i] = 0x0;
    }

    for (i = 0; i < 16; i++) {
        CPU.key_input[i] = 0x0;
        CPU.registers.V[i] = 0x0;
        CPU.registers.stack[i] = 0x0;
    }

    CPU.registers.delay_timer = 0x0;
    CPU.registers.sound_timer = 0x0;
    CPU.registers.I = 0x0;
    CPU.registers.stack_ptr = 0x0;
    CPU.registers.program_counter = 0x200;

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
        if (file_size == -1)
        {
            fclose(file_ptr);
            return -1;
        }
    }

    // Moves file pointer back to the file start
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

int main(int argc, char **argv) {

    // Initialize GLFW lib
    if (!glfwInit()) {
        printf("Failed to initialize GLFW.\n");
        return -1;
    }

    // Create window and it's OpenGL context
    GLFWwindow *win = glfwCreateWindow(64, 32, "Chip8 Emulator", NULL, NULL);

    if (!win) {
        glfwTerminate();
        printf("Failed to create a window.\n");
        return -1;
    }

    glfwMakeContextCurrent(win);

    chip8_cpu CPU = init();
    char *path = "Cave.ch8";
    load_rom(&CPU, path);
    uint16_t opcode, left_nibble, right_nibble, lowest_12_bits;
    uint8_t lower_high_byte, upper_low_byte, low_byte;

    // Main emulator loop
    while (!glfwWindowShouldClose(win)) {
        CPU.registers.program_counter += 2;
        // 8-bit shift to the left on the 8-byte sized region of memory pointed at by the pc to put it in the format 0xXX00
        left_nibble = CPU.memory[CPU.registers.program_counter] << 8;
        // Move the index by 1 (8 bytes) to get the second 8-byte sized part of the instruction as 0xXX
        right_nibble = CPU.memory[CPU.registers.program_counter + 1];
        // Perform bitwise OR on both to get the full 16 bytes instruction
        opcode = left_nibble | right_nibble;
        // Get the lower 4 bits (nibble/hex digit) of the high byte through a bitwise AND and a bitwise right shift by 1 byte
        lower_high_byte = (opcode & 0x0F00) >> 8;
        // Get the upper 4 bits (nibble/hex digit) of the low byte through a bitwise AND and a bitwise right shift by half a byte
        upper_low_byte = (opcode & 0x00F0) >> 4;
        // Get the low byte and the lowest 12 bits through bitwise AND
        lowest_12_bits = opcode & 0x0FFF;
        low_byte = opcode & 0x00FF;
        printf("Program counter address: %#x\n", CPU.registers.program_counter);
        printf("Opcode: %#x\n", opcode);
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
                break;
            case 0x1000:
                _1nnn(&CPU, lowest_12_bits);
                break;
            case 0x2000:
                _2nnn(&CPU, lowest_12_bits);
                break;
            case 0x3000:
                _3xkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0x4000:
                _4xkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0x5000:
                _5xy0(&CPU, lower_high_byte, upper_low_byte);
                break;
            case 0x6000:
                _6xkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0x7000:
                _7xkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0x8000:
                switch (opcode & 0x000F) {
                    case 0x0000:
                        _8xy0(&CPU, lower_high_byte, upper_low_byte);
                        break;
                    case 0x0001:
                        _8xy1(&CPU, lower_high_byte, upper_low_byte);
                        break;
                    case 0x0002:
                        _8xy2(&CPU, lower_high_byte, upper_low_byte);
                        break;
                    case 0x0003:
                        _8xy3(&CPU, lower_high_byte, upper_low_byte);
                        break;
                    case 0x0004:
                        _8xy4(&CPU, lower_high_byte, upper_low_byte);
                        break;
                    case 0x0005:
                        _8xy5(&CPU, lower_high_byte, upper_low_byte);
                        break;
                    case 0x0006:
                        _8xy6(&CPU, lower_high_byte, upper_low_byte);
                        break;
                    case 0x0007:
                        _8xy7(&CPU, lower_high_byte, upper_low_byte);
                        break;
                    case 0x000E:
                        _8xyE(&CPU, lower_high_byte, upper_low_byte);
                        break;
                }
                break;
            case 0x9000:
                _9xy0(&CPU, lower_high_byte, upper_low_byte);
                break;
            case 0xA000:
                _Annn(&CPU, lowest_12_bits);
                break;
            case 0xB000:
                _Bnnn(&CPU, lowest_12_bits);
                break;
            case 0xC000:
                _Cxkk(&CPU, lower_high_byte, low_byte);
                break;
            case 0xD000:
                _Dxyn(&CPU, lower_high_byte, upper_low_byte);
                break;
            case 0xE000:
                switch (opcode & 0x00FF) {
                    case 0x009E:
                        _Ex9E(&CPU, lower_high_byte);
                        break;
                    case 0x00A1:
                        _ExA1(&CPU, lower_high_byte);
                        break;
                }
                break;
            case 0xF000:
                switch (opcode & 0x00FF) {
                    case 0x0007:
                        _Fx07(&CPU, lower_high_byte);
                        break;
                    case 0x000A:
                        _Fx0A(&CPU, lower_high_byte);
                        break;
                    case 0x0015:
                        _Fx15(&CPU, lower_high_byte);
                        break;
                    case 0x0018:
                        _Fx18(&CPU, lower_high_byte);
                        break;
                    case 0x001E:
                        _Fx1E(&CPU, lower_high_byte);
                        break;
                    case 0x0029:
                        _Fx29(&CPU, lower_high_byte);
                        break;
                    case 0x0033:
                        _Fx33(&CPU, lower_high_byte);
                        break;
                    case 0x0055:
                        _Fx55(&CPU, lower_high_byte);
                        break;
                    case 0x0065:
                        _Fx65(&CPU, lower_high_byte);
                        break;
                }
                break;
            default:
                break;
        }
        
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(win);
        glfwPollEvents();

    }

    glfwDestroyWindow(win);
    glfwTerminate();

    return 0;
}