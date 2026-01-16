#include "axon_on.h"
#include <gui/gui.h>
#include <furi_hal_bt.h>
#include <extra_beacon.h>
#include <gui/elements.h>

#include "protocols/_protocols.h"

// Target OUI for Axon devices
#define TARGET_OUI "00:25:DF"

// Hacked together by @Willy-JL
// Custom adv API by @Willy-JL (idea by @xMasterX)
// iOS 17 Crash by @ECTO-1A
// Android, Samsung and Windows Pairs by @Spooks4576 and @ECTO-1A
// Research on behaviors and parameters by @Willy-JL, @ECTO-1A and @Spooks4576
// Controversy explained at https://willyjl.dev/blog/the-controversy-behind-apple-ble-spam
// Axon Cadabra by Kara

static Attack attacks[] = {
    {
        .title = "Axon Cadabra",
        .text = "Broadcast Axon recording command",
        .protocol = &protocol_axon,
        .payload =
            {
                .random_mac = false,
                .cfg =
                    {
                        .axon =
                            {
                                .mode = AxonModeCommand,
                                .fuzz_value = 0,
                            },
                    },
            },
    },
};

#define ATTACKS_COUNT ((signed)COUNT_OF(attacks))

static uint16_t delays[] = {20, 50, 100, 200, 500};

const NotificationSequence solid_message = {
    &message_red_0,
    &message_green_255,
    &message_blue_255,
    &message_do_not_reset,
    &message_delay_10,
    NULL,
};
NotificationMessage blink_message = {
    .type = NotificationMessageTypeLedBlinkStart,
    .data.led_blink.color = LightBlue | LightGreen,
    .data.led_blink.on_time = 10,
    .data.led_blink.period = 100,
};
const NotificationSequence blink_sequence = {
    &blink_message,
    &message_do_not_reset,
    NULL,
};

typedef struct {
    Ctx ctx;
    View* main_view;
    bool lock_warning;
    uint8_t lock_count;
    FuriTimer* lock_timer;

    bool advertising;
    uint8_t delay;
    GapExtraBeaconConfig config;
    FuriThread* thread;
    int8_t index;
} State;

// Note: Flipper Zero does not support BLE scanning
// This functionality would require scanning capabilities not available in current firmware

static void start_blink(State* state) {
    if(!state->ctx.led_indicator) return;
    uint16_t period = delays[state->delay];
    if(period <= 100) period += 30;
    blink_message.data.led_blink.period = period;
    notification_message_block(state->ctx.notification, &blink_sequence);
}
static void stop_blink(State* state) {
    if(!state->ctx.led_indicator) return;
    notification_message_block(state->ctx.notification, &sequence_blink_stop);
}

static void randomize_mac(State* state) {
    furi_hal_random_fill_buf(state->config.address, sizeof(state->config.address));
}

static void start_extra_beacon(State* state) {
    uint8_t size;
    uint8_t* packet;
    uint16_t delay = delays[state->delay];
    GapExtraBeaconConfig* config = &state->config;
    Payload* payload = &attacks[state->index].payload;
    const Protocol* protocol = attacks[state->index].protocol;

    config->min_adv_interval_ms = delay;
    config->max_adv_interval_ms = delay * 1.5;
    if(payload->random_mac) randomize_mac(state);
    furi_check(furi_hal_bt_extra_beacon_set_config(config));

    if(protocol) {
        protocol->make_packet(&size, &packet, payload);
    } else {
        // Fallback - shouldn't happen since we always have a protocol
        protocol_axon.make_packet(&size, &packet, payload);
    }
    furi_check(furi_hal_bt_extra_beacon_set_data(packet, size));
    free(packet);

    furi_check(furi_hal_bt_extra_beacon_start());
}

static int32_t adv_thread(void* _ctx) {
    State* state = _ctx;
    Payload* payload = &attacks[0].payload; // Always use first (only) attack
    if(!payload->random_mac) randomize_mac(state);
    start_blink(state);
    if(furi_hal_bt_extra_beacon_is_active()) {
        furi_check(furi_hal_bt_extra_beacon_stop());
    }

    while(state->advertising) {
        start_extra_beacon(state);
        furi_thread_flags_wait(true, FuriFlagWaitAny, delays[state->delay]);
        furi_check(furi_hal_bt_extra_beacon_stop());
    }

    stop_blink(state);
    return 0;
}

static void toggle_adv(State* state) {
    if(state->advertising) {
        state->advertising = false;
        furi_thread_flags_set(furi_thread_get_id(state->thread), true);
        furi_thread_join(state->thread);
    } else {
        state->advertising = true;
        furi_thread_start(state->thread);
    }
}

#define PAGE_MIN (-5)
#define PAGE_MAX 1
enum {
    PageHelpBruteforce = PAGE_MIN,
    PageHelpApps,
    PageHelpDelay,
    PageHelpDistance,
    PageHelpInfoConfig,
    PageStart = 0,
    PageEnd = 0, // Only one attack now
    PageAboutCredits = PAGE_MAX,
};

static void draw_callback(Canvas* canvas, void* _ctx) {
    State* state = *(State**)_ctx;

    // Always show Axon Cadabra (the only attack)
    const Attack* attack = &attacks[0];
    const Protocol* protocol = attack->protocol;

    canvas_set_font(canvas, FontSecondary);
    const Icon* icon = protocol ? protocol->icon : &I_axon_on;
    canvas_draw_icon(canvas, 4, 3, icon);
    canvas_draw_str(canvas, 14, 12, "AxonON");

    // Main display
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 30, "Trigger Axon Cameras");

    canvas_set_font(canvas, FontSecondary);
    // Draw custom positioned center button (moved 1 pixel right, 6 pixels down)
    elements_slightly_rounded_box(canvas, 32, 34, 64, 12);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, state->advertising ? "Stop" : "Start");
    canvas_set_color(canvas, ColorBlack);


    // Navigation buttons
    elements_button_left(canvas, "About");
    elements_button_right(canvas, "Credits");

    if(state->lock_warning) {
        canvas_set_font(canvas, FontSecondary);
        elements_bold_rounded_frame(canvas, 14, 8, 99, 48);
        elements_multiline_text(canvas, 65, 26, "To unlock\npress:");
        canvas_draw_icon(canvas, 65, 42, &I_Pin_back_arrow_10x8);
        canvas_draw_icon(canvas, 80, 42, &I_Pin_back_arrow_10x8);
        canvas_draw_icon(canvas, 95, 42, &I_Pin_back_arrow_10x8);
        canvas_draw_icon(canvas, 16, 13, &I_WarningDolphin_45x42);
        canvas_draw_dot(canvas, 17, 61);
    }
}

static bool input_callback(InputEvent* input, void* _ctx) {
    View* view = _ctx;
    State* state = *(State**)view_get_model(view);
    bool consumed = false;

    if(state->ctx.lock_keyboard) {
        consumed = true;
        state->lock_warning = true;
        if(state->lock_count == 0) {
            furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);
            furi_timer_start(state->lock_timer, 1000);
        }
        if(input->type == InputTypeShort && input->key == InputKeyBack) {
            state->lock_count++;
        }
        if(state->lock_count >= 3) {
            furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);
            furi_timer_start(state->lock_timer, 1);
        }
    } else if(input->type == InputTypeShort || input->type == InputTypeLong) {
        consumed = true;

        switch(input->key) {
        case InputKeyOk:
            if(input->type == InputTypeShort) {
                // Start/stop advertising Axon command
                toggle_adv(state);
            } else if(input->type == InputTypeLong) {
                // Go to config menu
                state->ctx.attack = &attacks[0];
                scene_manager_set_scene_state(state->ctx.scene_manager, SceneConfig, 0);
                view_commit_model(view, consumed);
                scene_manager_next_scene(state->ctx.scene_manager, SceneConfig);
                return consumed;
            }
            break;
        case InputKeyLeft:
            // Go to About screen
            scene_manager_set_scene_state(state->ctx.scene_manager, SceneAbout, 0);
            view_commit_model(view, consumed);
            scene_manager_next_scene(state->ctx.scene_manager, SceneAbout);
            return consumed;
        case InputKeyRight:
            // Go to Credits screen
            scene_manager_set_scene_state(state->ctx.scene_manager, SceneCredits, 0);
            view_commit_model(view, consumed);
            scene_manager_next_scene(state->ctx.scene_manager, SceneCredits);
            return consumed;
        case InputKeyBack:
            consumed = false;
            break;
        default:
            break;
        }
    }

    view_commit_model(view, consumed);
    return consumed;
}

static void lock_timer_callback(void* _ctx) {
    State* state = _ctx;
    if(state->lock_count < 3) {
        notification_message_block(state->ctx.notification, &sequence_display_backlight_off);
    } else {
        state->ctx.lock_keyboard = false;
    }
    with_view_model(state->main_view, State * *model, { (*model)->lock_warning = false; }, true);
    state->lock_count = 0;
    furi_timer_set_thread_priority(FuriTimerThreadPriorityNormal);
}

static bool custom_event_callback(void* _ctx, uint32_t event) {
    State* state = _ctx;
    return scene_manager_handle_custom_event(state->ctx.scene_manager, event);
}

static void tick_event_callback(void* _ctx) {
    State* state = _ctx;
    scene_manager_handle_tick_event(state->ctx.scene_manager);
}

static bool back_event_callback(void* _ctx) {
    State* state = _ctx;
    return scene_manager_handle_back_event(state->ctx.scene_manager);
}

int32_t axon_on(void* p) {
    UNUSED(p);
    GapExtraBeaconConfig prev_cfg;
    const GapExtraBeaconConfig* prev_cfg_ptr = furi_hal_bt_extra_beacon_get_config();
    if(prev_cfg_ptr) {
        memcpy(&prev_cfg, prev_cfg_ptr, sizeof(prev_cfg));
    }
    uint8_t prev_data[EXTRA_BEACON_MAX_DATA_SIZE];
    uint8_t prev_data_len = furi_hal_bt_extra_beacon_get_data(prev_data);
    bool prev_active = furi_hal_bt_extra_beacon_is_active();

    State* state = malloc(sizeof(State));
    state->config.adv_channel_map = GapAdvChannelMapAll;
    state->config.adv_power_level = GapAdvPowerLevel_6dBm;
    state->config.address_type = GapAddressTypePublic;
    state->thread = furi_thread_alloc();
    furi_thread_set_callback(state->thread, adv_thread);
    furi_thread_set_context(state->thread, state);
    furi_thread_set_stack_size(state->thread, 2048);
    state->ctx.led_indicator = true;
    state->lock_timer = furi_timer_alloc(lock_timer_callback, FuriTimerTypeOnce, state);

    state->ctx.notification = furi_record_open(RECORD_NOTIFICATION);
    Gui* gui = furi_record_open(RECORD_GUI);
    state->ctx.view_dispatcher = view_dispatcher_alloc();

    view_dispatcher_set_event_callback_context(state->ctx.view_dispatcher, state);
    view_dispatcher_set_custom_event_callback(state->ctx.view_dispatcher, custom_event_callback);
    view_dispatcher_set_tick_event_callback(state->ctx.view_dispatcher, tick_event_callback, 100);
    view_dispatcher_set_navigation_event_callback(state->ctx.view_dispatcher, back_event_callback);
    state->ctx.scene_manager = scene_manager_alloc(&scene_handlers, &state->ctx);

    state->main_view = view_alloc();
    view_allocate_model(state->main_view, ViewModelTypeLocking, sizeof(State*));
    with_view_model(state->main_view, State * *model, { *model = state; }, false);
    view_set_context(state->main_view, state->main_view);
    view_set_draw_callback(state->main_view, draw_callback);
    view_set_input_callback(state->main_view, input_callback);
    view_dispatcher_add_view(state->ctx.view_dispatcher, ViewMain, state->main_view);

    state->ctx.byte_input = byte_input_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher, ViewByteInput, byte_input_get_view(state->ctx.byte_input));

    state->ctx.submenu = submenu_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher, ViewSubmenu, submenu_get_view(state->ctx.submenu));

    state->ctx.text_input = text_input_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher, ViewTextInput, text_input_get_view(state->ctx.text_input));

    state->ctx.variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher,
        ViewVariableItemList,
        variable_item_list_get_view(state->ctx.variable_item_list));

    state->ctx.widget = widget_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher,
        ViewWidget,
        widget_get_view(state->ctx.widget));

    view_dispatcher_attach_to_gui(state->ctx.view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    scene_manager_next_scene(state->ctx.scene_manager, SceneMain);
    view_dispatcher_run(state->ctx.view_dispatcher);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewByteInput);
    byte_input_free(state->ctx.byte_input);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewSubmenu);
    submenu_free(state->ctx.submenu);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewTextInput);
    text_input_free(state->ctx.text_input);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewVariableItemList);
    variable_item_list_free(state->ctx.variable_item_list);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewWidget);
    widget_free(state->ctx.widget);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewMain);
    view_free(state->main_view);

    scene_manager_free(state->ctx.scene_manager);
    view_dispatcher_free(state->ctx.view_dispatcher);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);

    furi_timer_free(state->lock_timer);
    furi_thread_free(state->thread);
    free(state);

    if(furi_hal_bt_extra_beacon_is_active()) {
        furi_check(furi_hal_bt_extra_beacon_stop());
    }
    if(prev_cfg_ptr) {
        furi_check(furi_hal_bt_extra_beacon_set_config(&prev_cfg));
    }
    furi_check(furi_hal_bt_extra_beacon_set_data(prev_data, prev_data_len));
    if(prev_active) {
        furi_check(furi_hal_bt_extra_beacon_start());
    }
    return 0;
}
