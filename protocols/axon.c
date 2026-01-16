#include "_protocols.h"
#include "axon.h"
#include <furi_hal_bt.h>

// Service UUID 0xFE6C
#define SERVICE_UUID 0xFE6C
// Base service data from Android app - this is the command to start recording
static const uint8_t BASE_SERVICE_DATA[] = {
    0x01, 0x58, 0x38, 0x37, 0x30, 0x30, 0x32, 0x46,
    0x50, 0x34, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
    0xCE, 0x1B, 0x33, 0x00, 0x00, 0x02, 0x00, 0x00
};

static const char* axon_get_name(const Payload* payload) {
    switch(payload->cfg.axon.mode) {
    case AxonModeScan:
        return "Scan";
    case AxonModeCommand:
        return "Command";
    case AxonModeFuzz:
        return "Fuzz";
    default:
        return "Axon Cadabra";
    }
}

static void axon_make_packet(uint8_t* _size, uint8_t** _packet, Payload* payload) {
    AxonCfg* cfg = &payload->cfg.axon;

    // Start with base service data
    uint8_t service_data[24];
    memcpy(service_data, BASE_SERVICE_DATA, sizeof(BASE_SERVICE_DATA));

    // Apply fuzzing if enabled
    if(cfg->mode == AxonModeFuzz) {
        // Fuzz strategy: Modify bytes at different positions based on fuzz value
        // Position 10-11 seem to be command bytes (0x01 0x02 in original)
        // Position 20-21 also has values (0x00 0x02 in original)

        // Increment byte at position 10 (command byte 1)
        service_data[10] = ((cfg->fuzz_value >> 8) & 0xFF);
        // Increment byte at position 11 (command byte 2)
        service_data[11] = (cfg->fuzz_value & 0xFF);

        // Also vary the last few bytes
        service_data[20] = ((cfg->fuzz_value >> 4) & 0xFF);
        service_data[21] = ((cfg->fuzz_value << 4) & 0xFF);

        cfg->fuzz_value = (cfg->fuzz_value + 1) & 0xFFFF;
    }

    // Store current service data for display
    memcpy(cfg->current_service_data, service_data, sizeof(service_data));

    // Create the full packet with service UUID and data
    *_size = 2 + sizeof(service_data); // UUID (2 bytes) + data
    *_packet = malloc(*_size);

    // Service UUID (little endian)
    (*_packet)[0] = SERVICE_UUID & 0xFF;
    (*_packet)[1] = (SERVICE_UUID >> 8) & 0xFF;

    // Service data
    memcpy(*_packet + 2, service_data, sizeof(service_data));
}

static void axon_extra_config(Ctx* ctx) {
    UNUSED(ctx);
    // TODO: Add configuration options for fuzzing parameters
}

static uint8_t axon_config_count(const Payload* payload) {
    UNUSED(payload);
    return 0; // No extra config for now
}

const Protocol protocol_axon = {
    .icon = &I_axon_on,
    .get_name = axon_get_name,
    .make_packet = axon_make_packet,
    .extra_config = axon_extra_config,
    .config_count = axon_config_count,
};