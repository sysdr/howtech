/*
 * OpenOCD Configuration Validator and JTAG Analyzer
 * Validates OpenOCD config files and simulates JTAG timing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_LINE_LEN 1024
#define MAX_TAPS 8

typedef struct {
    char name[64];
    int irlen;
    uint32_t expected_id;
    bool ignore_version;
} jtag_tap_t;

typedef struct {
    char adapter_driver[32];
    int adapter_speed_khz;
    char reset_config[64];
    int tap_count;
    jtag_tap_t taps[MAX_TAPS];
    bool prefer_sba;
    bool enable_virtual;
    int command_timeout;
} openocd_config_t;

void trim(char *str) {
    char *start = str;
    while (isspace(*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
    
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) *end-- = '\0';
}

bool parse_config_line(const char *line, openocd_config_t *config) {
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim(buf);
    
    if (buf[0] == '#' || buf[0] == '\0') return true;
    
    if (strncmp(buf, "adapter driver", 14) == 0) {
        sscanf(buf, "adapter driver %31s", config->adapter_driver);
    } else if (strncmp(buf, "adapter speed", 13) == 0) {
        sscanf(buf, "adapter speed %d", &config->adapter_speed_khz);
    } else if (strncmp(buf, "reset_config", 12) == 0) {
        char *ptr = buf + 12;
        while (*ptr && isspace(*ptr)) ptr++;
        strncpy(config->reset_config, ptr, sizeof(config->reset_config) - 1);
        config->reset_config[sizeof(config->reset_config) - 1] = '\0';
    } else if (strncmp(buf, "jtag newtap", 11) == 0) {
        if (config->tap_count < MAX_TAPS) {
            jtag_tap_t *tap = &config->taps[config->tap_count];
            char *ptr = buf + 11;
            
            // Parse: jtag newtap chipname tapname -irlen N -expected-id 0xXXXXXXXX
            char chipname[32], tapname[32];
            if (sscanf(ptr, "%31s %31s", chipname, tapname) == 2) {
                snprintf(tap->name, sizeof(tap->name), "%s.%s", chipname, tapname);
                
                char *irlen_ptr = strstr(ptr, "-irlen");
                if (irlen_ptr && sscanf(irlen_ptr, "-irlen %d", &tap->irlen) == 1) {
                    char *id_ptr = strstr(ptr, "-expected-id");
                    if (id_ptr) {
                        sscanf(id_ptr, "-expected-id 0x%x", &tap->expected_id);
                    }
                    tap->ignore_version = (strstr(ptr, "-ignore-version") != NULL);
                    config->tap_count++;
                }
            }
        }
    } else if (strstr(buf, "set_prefer_sba on")) {
        config->prefer_sba = true;
    } else if (strstr(buf, "set_enable_virtual off")) {
        config->enable_virtual = false;
    } else if (strstr(buf, "set_command_timeout_sec")) {
        sscanf(buf, "%*s %*s %d", &config->command_timeout);
    }
    
    return true;
}

bool validate_config(const char *filename, openocd_config_t *config) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s\n", filename);
        return false;
    }
    
    memset(config, 0, sizeof(*config));
    config->adapter_speed_khz = 100; // default
    config->command_timeout = 10;
    
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        parse_config_line(line, config);
    }
    
    fclose(f);
    return true;
}

void print_config_summary(const openocd_config_t *config) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║          OpenOCD Configuration Summary                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Debug Adapter:\n");
    printf("  Driver:          %s\n", config->adapter_driver[0] ? config->adapter_driver : "(not specified)");
    printf("  JTAG Speed:      %d kHz\n", config->adapter_speed_khz);
    if (config->reset_config[0]) {
        printf("  Reset Config:    %s\n", config->reset_config);
    }
    
    printf("\nJTAG Chain:\n");
    printf("  TAP Count:       %d\n", config->tap_count);
    for (int i = 0; i < config->tap_count; i++) {
        const jtag_tap_t *tap = &config->taps[i];
        printf("  TAP %d: %s\n", i + 1, tap->name);
        printf("    IR Length:     %d bits\n", tap->irlen);
        printf("    Expected IDCODE: 0x%08x\n", tap->expected_id);
        printf("    Ignore Version:  %s\n", tap->ignore_version ? "yes" : "no");
        
        // Calculate JTAG timing
        float clock_period_us = 1000.0f / config->adapter_speed_khz;
        float ir_scan_time_us = tap->irlen * clock_period_us;
        float dmi_scan_time_us = 41 * clock_period_us; // DMI is 41 bits
        
        printf("    IR Scan Time:  %.1f μs (%d clocks @ %.1f MHz)\n", 
               ir_scan_time_us, tap->irlen, config->adapter_speed_khz / 1000.0f);
        printf("    DMI Scan Time: %.1f μs (41 clocks)\n", dmi_scan_time_us);
    }
    
    printf("\nRISC-V Debug Module:\n");
    printf("  Prefer System Bus Access: %s\n", config->prefer_sba ? "yes" : "no");
    printf("  Virtual Memory:           %s\n", config->enable_virtual ? "enabled" : "disabled");
    printf("  Command Timeout:          %d seconds\n", config->command_timeout);
}

void validate_jtag_timing(const openocd_config_t *config) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║          JTAG Timing Analysis                              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    float clock_period_us = 1000.0f / config->adapter_speed_khz;
    float clock_period_ns = clock_period_us * 1000.0f;
    
    printf("JTAG Clock Configuration:\n");
    printf("  Frequency:       %.2f MHz (%d kHz)\n", 
           config->adapter_speed_khz / 1000.0f, config->adapter_speed_khz);
    printf("  Period:          %.1f ns\n", clock_period_ns);
    
    // Check against typical requirements
    const float min_setup_ns = 10.0f;
    const float min_hold_ns = 5.0f;
    const float max_freq_mhz = 10.0f;
    
    printf("\nTiming Validation:\n");
    if (clock_period_ns < (min_setup_ns + min_hold_ns)) {
        printf("  ⚠ WARNING: Clock period (%.1f ns) may be too fast!\n", clock_period_ns);
        printf("             Minimum: %.1f ns (setup %.1f ns + hold %.1f ns)\n", 
               min_setup_ns + min_hold_ns, min_setup_ns, min_hold_ns);
    } else {
        printf("  ✓ Clock period adequate for timing requirements\n");
    }
    
    if (config->adapter_speed_khz / 1000.0f > max_freq_mhz) {
        printf("  ⚠ WARNING: Frequency (%.2f MHz) exceeds typical max (%.2f MHz)\n",
               config->adapter_speed_khz / 1000.0f, max_freq_mhz);
        printf("             May cause signal integrity issues without proper termination\n");
    } else if (config->adapter_speed_khz < 100) {
        printf("  ⚠ INFO: Very conservative speed (< 100 kHz) - good for bring-up\n");
    } else {
        printf("  ✓ Frequency within recommended range\n");
    }
    
    // Calculate operation times
    printf("\nOperation Timing Estimates:\n");
    float register_read_ms = (41 * 3 * clock_period_us) / 1000.0f; // 3 scans for DMI read
    float memory_read_64b_ms = (41 * 6 * clock_period_us) / 1000.0f; // 6 ops for 64-bit read
    
    printf("  Single Register Read:    %.2f ms\n", register_read_ms);
    printf("  64-bit Memory Read:      %.2f ms\n", memory_read_64b_ms);
    printf("  1KB Memory Dump:         ~%.1f ms (16 x 64-bit reads)\n", memory_read_64b_ms * 16);
}

void check_common_errors(const openocd_config_t *config) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║          Common Configuration Issues Check                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    bool found_issues = false;
    
    if (config->tap_count == 0) {
        printf("✗ CRITICAL: No JTAG TAPs defined!\n");
        printf("  Add 'jtag newtap' declaration with correct IR length and IDCODE\n\n");
        found_issues = true;
    }
    
    for (int i = 0; i < config->tap_count; i++) {
        const jtag_tap_t *tap = &config->taps[i];
        if (tap->irlen < 2 || tap->irlen > 32) {
            printf("✗ ERROR: TAP %s has suspicious IR length (%d bits)\n", tap->name, tap->irlen);
            printf("  Typical RISC-V: 5 bits. Check your datasheet!\n\n");
            found_issues = true;
        }
        if (tap->expected_id == 0) {
            printf("⚠ WARNING: TAP %s has no expected IDCODE\n", tap->name);
            printf("  Add -expected-id to catch wrong chips or connection issues\n\n");
            found_issues = true;
        }
    }
    
    if (config->adapter_speed_khz > 2000) {
        printf("⚠ WARNING: JTAG speed > 2 MHz\n");
        printf("  High speeds require clean signals. Start at 100-200 kHz for bring-up.\n\n");
        found_issues = true;
    }
    
    if (config->reset_config[0] == '\0') {
        printf("⚠ WARNING: No reset_config specified\n");
        printf("  Add 'reset_config trst_and_srst separate' or appropriate variant\n\n");
        found_issues = true;
    }
    
    if (!found_issues) {
        printf("✓ No obvious configuration issues detected\n");
        printf("  Note: This validator cannot catch all problems. Test with real hardware!\n");
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }
    
    openocd_config_t config;
    if (!validate_config(argv[1], &config)) {
        return 1;
    }
    
    print_config_summary(&config);
    validate_jtag_timing(&config);
    check_common_errors(&config);
    
    printf("\n");
    return 0;
}
