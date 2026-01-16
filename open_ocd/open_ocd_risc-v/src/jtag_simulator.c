/*
 * JTAG TAP State Machine Simulator
 * Demonstrates TAP state transitions and timing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

typedef enum {
    TEST_LOGIC_RESET = 0,
    RUN_TEST_IDLE,
    SELECT_DR_SCAN,
    CAPTURE_DR,
    SHIFT_DR,
    EXIT1_DR,
    PAUSE_DR,
    EXIT2_DR,
    UPDATE_DR,
    SELECT_IR_SCAN,
    CAPTURE_IR,
    SHIFT_IR,
    EXIT1_IR,
    PAUSE_IR,
    EXIT2_IR,
    UPDATE_IR
} tap_state_t;

const char *state_names[] = {
    "TEST-LOGIC-RESET", "RUN-TEST/IDLE",
    "SELECT-DR-SCAN", "CAPTURE-DR", "SHIFT-DR", "EXIT1-DR", "PAUSE-DR", "EXIT2-DR", "UPDATE-DR",
    "SELECT-IR-SCAN", "CAPTURE-IR", "SHIFT-IR", "EXIT1-IR", "PAUSE-IR", "EXIT2-IR", "UPDATE-IR"
};

tap_state_t tap_state = TEST_LOGIC_RESET;

tap_state_t tap_transition(tap_state_t current, bool tms) {
    switch (current) {
        case TEST_LOGIC_RESET:
            return tms ? TEST_LOGIC_RESET : RUN_TEST_IDLE;
        case RUN_TEST_IDLE:
            return tms ? SELECT_DR_SCAN : RUN_TEST_IDLE;
        case SELECT_DR_SCAN:
            return tms ? SELECT_IR_SCAN : CAPTURE_DR;
        case CAPTURE_DR:
            return tms ? EXIT1_DR : SHIFT_DR;
        case SHIFT_DR:
            return tms ? EXIT1_DR : SHIFT_DR;
        case EXIT1_DR:
            return tms ? UPDATE_DR : PAUSE_DR;
        case PAUSE_DR:
            return tms ? EXIT2_DR : PAUSE_DR;
        case EXIT2_DR:
            return tms ? UPDATE_DR : SHIFT_DR;
        case UPDATE_DR:
            return tms ? SELECT_DR_SCAN : RUN_TEST_IDLE;
        case SELECT_IR_SCAN:
            return tms ? TEST_LOGIC_RESET : CAPTURE_IR;
        case CAPTURE_IR:
            return tms ? EXIT1_IR : SHIFT_IR;
        case SHIFT_IR:
            return tms ? EXIT1_IR : SHIFT_IR;
        case EXIT1_IR:
            return tms ? UPDATE_IR : PAUSE_IR;
        case PAUSE_IR:
            return tms ? EXIT2_IR : PAUSE_IR;
        case EXIT2_IR:
            return tms ? UPDATE_IR : SHIFT_IR;
        case UPDATE_IR:
            return tms ? SELECT_DR_SCAN : RUN_TEST_IDLE;
        default:
            return TEST_LOGIC_RESET;
    }
}

void print_tap_state(tap_state_t state) {
    printf("%-20s", state_names[state]);
}

void simulate_ir_scan(int irlen, uint32_t ir_value) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║          IR Scan Simulation (%d bits)                     ║\n", irlen);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    bool tms_sequence[] = {1, 1, 0, 0}; // IDLE → SELECT-DR → SELECT-IR → CAPTURE-IR
    
    printf("%-6s %-5s %-20s\n", "Clock", "TMS", "TAP State");
    printf("─────────────────────────────────────────────────\n");
    
    int clock = 0;
    for (int i = 0; i < 4; i++) {
        printf("%-6d %-5d ", clock++, tms_sequence[i]);
        tap_state = tap_transition(tap_state, tms_sequence[i]);
        print_tap_state(tap_state);
        printf("\n");
        usleep(100000); // 100ms delay for visualization
    }
    
    printf("\nShifting IR value 0x%x (%d bits)...\n", ir_value, irlen);
    for (int i = 0; i < irlen; i++) {
        bool tms = (i == irlen - 1) ? 1 : 0; // TMS=1 on last bit
        bool tdi = (ir_value >> i) & 1;
        
        printf("%-6d %-5d ", clock++, tms);
        tap_state = tap_transition(tap_state, tms);
        print_tap_state(tap_state);
        printf(" TDI=%d\n", tdi);
        usleep(50000);
    }
    
    // Exit to UPDATE-IR and back to IDLE
    bool exit_sequence[] = {1, 0};
    for (int i = 0; i < 2; i++) {
        printf("%-6d %-5d ", clock++, exit_sequence[i]);
        tap_state = tap_transition(tap_state, exit_sequence[i]);
        print_tap_state(tap_state);
        printf("\n");
        usleep(100000);
    }
    
    printf("\nIR scan complete. Total clocks: %d\n", clock);
}

void simulate_dr_scan(int drlen) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║          DR Scan Simulation (%d bits)                     ║\n", drlen);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Assuming we're in IDLE, go to SHIFT-DR
    bool tms_sequence[] = {1, 0, 0}; // IDLE → SELECT-DR → CAPTURE-DR
    
    printf("%-6s %-5s %-20s\n", "Clock", "TMS", "TAP State");
    printf("─────────────────────────────────────────────────\n");
    
    int clock = 0;
    for (int i = 0; i < 3; i++) {
        printf("%-6d %-5d ", clock++, tms_sequence[i]);
        tap_state = tap_transition(tap_state, tms_sequence[i]);
        print_tap_state(tap_state);
        printf("\n");
        usleep(100000);
    }
    
    printf("\nShifting %d bits through DR...\n", drlen);
    for (int i = 0; i < drlen; i++) {
        bool tms = (i == drlen - 1) ? 1 : 0;
        
        printf("%-6d %-5d ", clock++, tms);
        tap_state = tap_transition(tap_state, tms);
        print_tap_state(tap_state);
        printf("\n");
        
        if (i % 10 == 0) usleep(50000);
    }
    
    // Exit to UPDATE-DR and back to IDLE
    bool exit_sequence[] = {1, 0};
    for (int i = 0; i < 2; i++) {
        printf("%-6d %-5d ", clock++, exit_sequence[i]);
        tap_state = tap_transition(tap_state, exit_sequence[i]);
        print_tap_state(tap_state);
        printf("\n");
        usleep(100000);
    }
    
    printf("\nDR scan complete. Total clocks: %d\n", clock);
}

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║       JTAG TAP State Machine Simulator                     ║\n");
    printf("║       Demonstrates DMI Register Access Sequence            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    // Reset to known state
    printf("\nResetting TAP to TEST-LOGIC-RESET (5 TMS=1 clocks)...\n");
    for (int i = 0; i < 5; i++) {
        tap_state = tap_transition(tap_state, 1);
    }
    tap_state = tap_transition(tap_state, 0); // Go to IDLE
    printf("TAP State: %s\n", state_names[tap_state]);
    
    sleep(1);
    
    // Simulate IR scan to load DTM command (0x11)
    simulate_ir_scan(5, 0x11);
    
    sleep(1);
    
    // Simulate DR scan to access DMI register (41 bits)
    simulate_dr_scan(41);
    
    printf("\n════════════════════════════════════════════════════════════\n");
    printf("Simulation complete!\n");
    printf("This demonstrates the JTAG operations OpenOCD performs\n");
    printf("to access RISC-V Debug Module registers.\n");
    printf("════════════════════════════════════════════════════════════\n\n");
    
    return 0;
}
