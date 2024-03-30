#include <furi.h>
#include <furi_hal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gui/gui.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

// Create enum for screen states
typedef enum {
    GuessScreenMain,
    GuessScreenSelectNumber,
    GuessScreenGetNumber,
} GuessGameScreenState;

// Create struct for MainApp
typedef struct {
    FuriMessageQueue* event_queue;
    Gui* gui;
    ViewPort* view_port;
    FuriMutex** mutex;
    NotificationApp* not_app;
    int remaining_lives;
    int selected_number;
    int get_number;
    int screen_state;
    char* selected_number_str;
    char* get_nummber_str;
    char* remaining_lives_str;
} MainApp;

// Convert int to char*
char* convert_int2charp(int target_int) {
    char* temp = malloc(100 * sizeof(char));
    snprintf(temp, 100, "%d", target_int);
    return temp;
}

// Generate random number
int generate_random_number() {
    int temp;
    temp = rand() % 10 + 1;
    return temp;
}

// Create screen callbacks
static void main_screen_callback(Canvas* canvas, void* ctx) {
    MainApp* app = ctx;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    notification_message(app->not_app, &sequence_reset_rgb);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 33, 15, "Welcome");
    canvas_draw_str(canvas, 55, 27, "to");
    canvas_draw_str(canvas, 12, 40, " - Guess Game -");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 17, 60, "By @CYB3RMX");
    app->screen_state = GuessScreenMain;
    furi_mutex_release(app->mutex);
}
static void select_number_callback(Canvas* canvas, void* ctx) {
    MainApp* app = ctx;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    notification_message(app->not_app, &sequence_reset_rgb);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 1, 10, "Select Number Between 1-10");
    canvas_draw_frame(canvas, 20, 20, 20, 20);
    app->selected_number_str = convert_int2charp(app->selected_number);
    canvas_draw_str_aligned(canvas, 30, 30, AlignCenter, AlignCenter, app->selected_number_str);
    canvas_draw_str(canvas, 45, 30, "Up: Increase");
    canvas_draw_str(canvas, 45, 40, "Down: Decrease");
    canvas_draw_str(canvas, 45, 50, "OK: Select");
    app->screen_state = GuessScreenSelectNumber;
    free(app->selected_number_str);
    furi_mutex_release(app->mutex);
}
static void get_number_callback(Canvas* canvas, void* ctx) {
    MainApp* app = ctx;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 3, 20, "Remaining lives: ");
    app->remaining_lives_str = convert_int2charp(app->remaining_lives);
    canvas_draw_str(canvas, 80, 20, app->remaining_lives_str);

    canvas_draw_str(canvas, 3, 30, "Selected Number: ");
    app->selected_number_str = convert_int2charp(app->selected_number);
    canvas_draw_str(canvas, 80, 30, app->selected_number_str);

    canvas_draw_str(canvas, 3, 40, "Answer: ");
    app->get_nummber_str = convert_int2charp(app->get_number);
    canvas_draw_str(canvas, 80, 40, app->get_nummber_str);

    if(app->remaining_lives != 0) {
        canvas_draw_str(canvas, 10, 60, "Press OK to continue");
    } else {
        canvas_draw_str(canvas, 5, 60, "You lose!");
    }
    app->screen_state = GuessScreenGetNumber;
    free(app->selected_number_str);
    free(app->remaining_lives_str);
    free(app->get_nummber_str);
    furi_mutex_release(app->mutex);
}

// Create input callback
static void input_callback(InputEvent* input_event, void* ctx) {
    MainApp* app = ctx;
    if(input_event->type == InputTypeShort) {
        furi_message_queue_put(app->event_queue, input_event, 0);
    }
}

// Init MainApp
MainApp* init_main_app() {
    MainApp* app = malloc(sizeof(MainApp)); // Allocate memory for MainApp struct
    app->event_queue =
        furi_message_queue_alloc(8, sizeof(InputEvent)); // Create queue for key inputs/events
    app->gui = furi_record_open(RECORD_GUI); // Init Gui object
    app->view_port = view_port_alloc(); // Allocate memory for ViewPort
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal); // Create mutex for callback handling
    app->screen_state = GuessScreenMain; // Main screen for default
    app->not_app = furi_record_open(RECORD_NOTIFICATION);
    app->selected_number = 1;
    app->remaining_lives = 3;
    view_port_draw_callback_set(app->view_port, main_screen_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
    return app;
}

// Free everything
void free_everything(MainApp* app) {
    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->mutex);
    free(app);
}

int32_t guess_game_app_main(void* p) {
    UNUSED(p);
    MainApp* app = init_main_app();

    // Create input event
    InputEvent input_evt;

    // Handle keystrokes
    while(furi_message_queue_get(app->event_queue, &input_evt, FuriWaitForever) == FuriStatusOk) {
        furi_mutex_acquire(app->mutex, FuriWaitForever);

        // Navigate over screens
        if(input_evt.key == InputKeyRight) {
            if(app->screen_state == GuessScreenMain) {
                view_port_draw_callback_set(app->view_port, select_number_callback, app);
            }
        }
        if(input_evt.key == InputKeyBack) {
            break;
        }

        // Increase/Decrease/Select
        if(input_evt.key == InputKeyUp) {
            if(app->screen_state == GuessScreenSelectNumber) {
                if(app->selected_number > 10) {
                    app->selected_number = 10;
                } else {
                    app->selected_number++;
                }
            }
        }
        if(input_evt.key == InputKeyDown) {
            if(app->screen_state == GuessScreenSelectNumber) {
                if(app->selected_number < 1) {
                    app->selected_number = 1;
                } else {
                    app->selected_number--;
                }
            }
        }
        if(input_evt.key == InputKeyOk) {
            if(app->screen_state == GuessScreenSelectNumber &&
               (app->selected_number >= 1 && app->selected_number <= 10)) {
                app->get_number = generate_random_number();
                if(app->get_number != app->selected_number) {
                    app->remaining_lives--;
                    notification_message(app->not_app, &sequence_set_red_255);
                    notification_message(app->not_app, &sequence_error);
                } else {
                    app->remaining_lives++;
                    notification_message(app->not_app, &sequence_set_green_255);
                    notification_message(app->not_app, &sequence_success);
                }
                view_port_draw_callback_set(app->view_port, get_number_callback, app);
            }
            if(app->screen_state == GuessScreenGetNumber) {
                if(app->remaining_lives != 0) {
                    view_port_draw_callback_set(app->view_port, select_number_callback, app);
                } else {
                    view_port_draw_callback_set(app->view_port, main_screen_callback, app);
                    app->remaining_lives = 3;
                }
            }
        }
        furi_mutex_release(app->mutex);
    }
    free_everything(app);
    return 0;
}
