#include <stdio.h>
#include <stdlib.h>
#include <furi.h>
#include <gui/gui.h>

// Create enum for app state
typedef enum {
    AppOnMainScreen,
    AppOnCounterConfigScreen,
    AppOnCounterScreen,
} AppStateEnum;

// Create enum for counter config
typedef enum {
    CounterIncrease,
    CounterDecrease,
} CounterConfigEnum;

// Create struct for main app
typedef struct {
    FuriMutex** mutex;
    ViewPort* view_port;
    Gui* gui;
    FuriMessageQueue* event_queue;
    int screen_state;
    int counter;
    char* counter_str;
    int counter_config_state;
} MainApp;

char* convert_int2charp(MainApp* app) {
    char* temp = malloc(100 * sizeof(char));
    snprintf(temp, 100, "%d", app->counter);
    return temp;
}

// Screen callbacks
static void main_screen_callback(Canvas* canvas, void* ctx) {
    MainApp* app = ctx;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 33, 15, "Welcome");
    canvas_draw_str(canvas, 55, 27, "to");
    canvas_draw_str(canvas, 12, 40, "SimpleCounter App!");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 17, 60, "By @CYB3RMX");
    app->screen_state = AppOnMainScreen;
    furi_mutex_release(app->mutex);
}
static void counter_render_callback(Canvas* canvas, void* ctx) {
    MainApp* app = ctx;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 30, 10, "Counter Screen");
    canvas_draw_str(canvas, 5, 30, "Counter: ");
    app->counter_str = convert_int2charp(app);
    canvas_draw_str(canvas, 45, 30, app->counter_str);
    app->screen_state = AppOnCounterScreen;
    free(app->counter_str);
    furi_mutex_release(app->mutex);
}
static void counter_config_screen_callback(Canvas* canvas, void* ctx) {
    MainApp* app = ctx;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 30, 10, "Counter Config");
    canvas_draw_frame(canvas, 5, 20, 60, 15);
    canvas_draw_str_aligned(canvas, 32, 27, AlignCenter, AlignCenter, "increase");
    canvas_draw_frame(canvas, 5, 40, 60, 15);
    canvas_draw_str_aligned(canvas, 32, 47, AlignCenter, AlignCenter, "decrease");
    if(app->counter_config_state == CounterIncrease) {
        canvas_draw_str_aligned(canvas, 85, 27, AlignCenter, AlignCenter, "<---");
    } else {
        canvas_draw_str_aligned(canvas, 85, 46, AlignCenter, AlignCenter, "<---");
    }
    app->screen_state = AppOnCounterConfigScreen;
    furi_mutex_release(app->mutex);
}

// Input callback
static void input_callback(InputEvent* input_evt, void* ctx) {
    MainApp* app = ctx;
    if(input_evt->type == InputTypeShort) {
        furi_message_queue_put(app->event_queue, input_evt, 0);
    }
}

// Create and destroy app object
MainApp* init_app() {
    MainApp* app = malloc(sizeof(MainApp));
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->view_port = view_port_alloc();
    app->gui = furi_record_open(RECORD_GUI);
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->screen_state = AppOnMainScreen;
    app->counter = 0;
    app->counter_config_state = CounterIncrease; // By default
    view_port_draw_callback_set(app->view_port, main_screen_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
    return app;
}
void free_everything(MainApp* app) {
    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->mutex);
    free(app);
}

int32_t simple_counter_main(void* p) {
    UNUSED(p);
    MainApp* app = init_app();

    // Create input event
    InputEvent input_event;

    // Handle keystrokes
    while(furi_message_queue_get(app->event_queue, &input_event, FuriWaitForever) ==
          FuriStatusOk) {
        furi_mutex_acquire(app->mutex, FuriWaitForever);
        // Navigate over screens
        if(input_event.key == InputKeyRight) {
            if(app->screen_state == AppOnMainScreen) {
                view_port_draw_callback_set(app->view_port, counter_config_screen_callback, app);
            }
            if(app->screen_state == AppOnCounterConfigScreen) {
                view_port_draw_callback_set(app->view_port, counter_render_callback, app);
            }
        }
        // Interact with counter config screen
        if(input_event.key == InputKeyDown) {
            if(app->screen_state == AppOnCounterConfigScreen) {
                if(app->counter_config_state == CounterIncrease) {
                    app->counter_config_state = CounterDecrease;
                }
            }
        }
        if(input_event.key == InputKeyUp) {
            if(app->screen_state == AppOnCounterConfigScreen) {
                if(app->counter_config_state == CounterDecrease) {
                    app->counter_config_state = CounterIncrease;
                }
            }
        }
        // Interact with counter screen
        if(input_event.key == InputKeyOk) {
            if(app->screen_state == AppOnCounterScreen) {
                if(app->counter_config_state == CounterIncrease) {
                    app->counter++;
                    view_port_draw_callback_set(app->view_port, counter_render_callback, app);
                } else {
                    app->counter--;
                    view_port_draw_callback_set(app->view_port, counter_render_callback, app);
                }
            }
        }
        // Return to main screen
        if(input_event.key == InputKeyBack) {
            if(app->screen_state != AppOnMainScreen) {
                if(app->screen_state == AppOnCounterScreen) {
                    view_port_draw_callback_set(
                        app->view_port, counter_config_screen_callback, app);
                }
                if(app->screen_state == AppOnCounterConfigScreen) {
                    view_port_draw_callback_set(app->view_port, main_screen_callback, app);
                }
            } else {
                break;
            }
        }
        furi_mutex_release(app->mutex);
    }

    // Free memory
    free_everything(app);
    return 0;
}
