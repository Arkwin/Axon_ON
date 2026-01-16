#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assets_icons.h>
#include "axon_on_icons.h"
#include <furi_hal_random.h>
#include <core/core_defines.h>
#include "../axon_on.h"

typedef enum {
    AxonModeScan,
    AxonModeCommand,
    AxonModeFuzz,
} AxonMode;

typedef struct {
    AxonMode mode;
    uint16_t fuzz_value;
    uint8_t current_service_data[24];
} AxonCfg;