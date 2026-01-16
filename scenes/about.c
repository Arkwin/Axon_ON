#include "../axon_on.h"

void scene_about_on_enter(void* context) {
    Ctx* ctx = context;
    view_dispatcher_switch_to_view(ctx->view_dispatcher, ViewWidget);
    widget_add_text_box_element(
        ctx->widget,
        0,
        0,
        128,
        64,
        AlignCenter,
        AlignCenter,
        "This app was created from the work of AxonCadabra and is used to trigger Axon cameras to start recording",
        false);
}

bool scene_about_on_event(void* context, SceneManagerEvent event) {
    Ctx* ctx = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = true;
        scene_manager_previous_scene(ctx->scene_manager);
    }

    return consumed;
}

void scene_about_on_exit(void* context) {
    Ctx* ctx = context;
    widget_reset(ctx->widget);
}