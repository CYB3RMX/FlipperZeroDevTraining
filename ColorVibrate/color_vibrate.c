#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

char* notificationMessage;

static void screen_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 30, notificationMessage);
}

static void input_callback(InputEvent* ievt, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* evt_queue = ctx;
    furi_message_queue_put(evt_queue, ievt, FuriWaitForever);
}

static void notification_callback(int key_type) {
    NotificationApp* notapp = furi_record_open(RECORD_NOTIFICATION);
    if(key_type == InputKeyUp) {
        notificationMessage = "Pressed - UP";
        notification_message(notapp, &sequence_set_blue_255);
        furi_hal_vibro_on(true);
        furi_delay_ms(100);
        furi_hal_vibro_on(false);
    }
    if(key_type == InputKeyDown) {
        notificationMessage = "Pressed - DOWN";
        notification_message(notapp, &sequence_blink_magenta_100);
        furi_hal_vibro_on(true);
        furi_delay_ms(100);
        furi_hal_vibro_on(false);
    }
    if(key_type == InputKeyLeft) {
        notificationMessage = "Pressed - LEFT";
        notification_message(notapp, &sequence_set_red_255);
        furi_hal_vibro_on(true);
        furi_delay_ms(100);
        furi_hal_vibro_on(false);
    }
    if(key_type == InputKeyRight) {
        notificationMessage = "Pressed - RIGHT";
        notification_message(notapp, &sequence_set_green_255);
        furi_hal_vibro_on(true);
        furi_delay_ms(100);
        furi_hal_vibro_on(false);
    }
    if(key_type == InputKeyOk) {
        notificationMessage = "Pressed - OK ;)";
        notification_message(notapp, &sequence_blink_yellow_100);
        furi_hal_vibro_on(true);
        furi_delay_ms(1000);
        furi_hal_vibro_on(false);
    }
    notification_message(notapp, &sequence_reset_rgb);
    return;
}

int32_t color_vibrate_main(void* p) {
    UNUSED(p);

    // Hello message
    notificationMessage = "Welcome to ColorVibrate App!";

    // Create Input event
    InputEvent input_event;

    // Message queue
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Create viewport for gui app
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, screen_callback, NULL);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    // Register view_port into gui
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Handle key press
    while(true) {
        furi_check(
            furi_message_queue_get(event_queue, &input_event, FuriWaitForever) == FuriStatusOk);
        if(input_event.key == InputKeyBack) {
            break;
        } else {
            notification_callback(input_event.key);
        }
    }

    // Free memory
    furi_message_queue_free(event_queue);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    return 0;
}
