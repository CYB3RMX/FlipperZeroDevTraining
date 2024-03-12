#include <furi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <furi_hal.h>
#include <gui/gui.h>

char* intToHex(int num) {
    // Check if the number is zero
    if(num == 0) {
        char* hex = (char*)malloc(3 * sizeof(char));
        if(hex == NULL) {
            printf("Memory allocation failed\n");
        }
        return hex;
    }

    // Maximum size of hex number in 32-bit system
    char* hex = (char*)malloc(10 * sizeof(char)); // 8 characters plus '0x' and '\0'
    if(hex == NULL) {
        printf("Memory allocation failed\n");
    }

    // Loop to convert each digit
    int i = 0;
    while(num != 0) {
        int remainder = num % 16;
        if(remainder < 10)
            hex[i++] = remainder + 48; // Convert to ASCII
        else
            hex[i++] = remainder + 55; // Convert to ASCII
        num /= 16;
    }

    // Reverse the array to get the correct order
    int j;
    for(j = 0; j < i / 2; j++) {
        char temp = hex[j];
        hex[j] = hex[i - j - 1];
        hex[i - j - 1] = temp;
    }

    return hex;
}

static void screen_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas); // Clear screen
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 10, 10, "- BLEapp Main Menu -");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 25, "LEFT -> MAIN MENU");
    canvas_draw_str(canvas, 2, 35, "RIGHT -> Bluetooth Info");
    canvas_draw_str(canvas, 30, 50, "By @CYB3RMX");
}

static void blinfo_screen(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);

    // Get ble device name
    const char* bl_device_name = furi_hal_version_get_ble_local_device_name_ptr();

    // Print info on screen
    canvas_draw_str(canvas, 28, 10, "- Bluetooth Info -");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 25, "Device Name: ");
    canvas_draw_str(canvas, 60, 25, bl_device_name);

    // Check alive state
    canvas_draw_str(canvas, 2, 35, "Alive: ");
    if(furi_hal_bt_is_alive() == true) {
        canvas_draw_str(canvas, 25, 35, "True");
    } else {
        canvas_draw_str(canvas, 25, 35, "False");
    }

    // Check active state
    canvas_draw_str(canvas, 2, 45, "Active: ");
    if(furi_hal_bt_is_active() == true) {
        canvas_draw_str(canvas, 33, 45, "True");
    } else {
        canvas_draw_str(canvas, 33, 45, "False");
    }

    // Get mac addr Thanks to @MathematicianGoat for helping this coding hell!!
    canvas_draw_str(canvas, 2, 55, "MAC: ");
    const uint8_t* ble_mac_addr = furi_hal_version_get_ble_mac();
    char* mac_addr_str = (char*)ble_mac_addr;
    int x_start = 25;
    for(size_t i = 0; i <= 13 - 1; i++) {
        char* temp = intToHex((int)mac_addr_str[i]);
        canvas_draw_str(canvas, x_start, 55, temp);
        x_start = x_start + 14;
        free(temp);
    }
}

static void input_callback(InputEvent* input_evt, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_evt, FuriWaitForever);
}

int32_t bleapp_main(void* p) {
    UNUSED(p);

    // Create Input event
    InputEvent input_evt;

    // Create MessageQueue
    FuriMessageQueue* event = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Create view_port
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, screen_callback, NULL); // Main menu for default
    view_port_input_callback_set(view_port, input_callback, event);

    // Create gui
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Handle input events
    while(true) {
        furi_check(furi_message_queue_get(event, &input_evt, FuriWaitForever) == FuriStatusOk);
        if(input_evt.key == InputKeyRight) {
            view_port_draw_callback_set(view_port, blinfo_screen, NULL);
        }
        if(input_evt.key == InputKeyLeft) {
            view_port_draw_callback_set(view_port, screen_callback, NULL);
        }
        if(input_evt.key == InputKeyBack) {
            break;
        }
    }

    // Free memory
    furi_message_queue_free(event);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    return 0;
}
