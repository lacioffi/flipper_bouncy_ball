#include <furi.h>
#include <furi_hal.h>
#include <notification/notification.h>
#include <gui/gui.h>
#include <bouncy_icons.h>

struct app_context {
    float ball_x, ball_y, last_ball_x, last_ball_y;
    float ball_speed_x, ball_speed_y;
    bool ghost_active;
};

static void hello_app_draw_callback(Canvas* canvas, void* ctx) {
    struct app_context* app_context = ctx;

    canvas_set_bitmap_mode(canvas, true);
    //Frame
    canvas_draw_frame(canvas, 0, 0, 128, 64);
    //Ball
    canvas_draw_disc(canvas, (int)app_context->ball_x, (int)app_context->ball_y, 3);
    //Ghost ball / Trailing effect
    if(app_context->ghost_active == true) {
        canvas_draw_disc(canvas, (int)app_context->last_ball_x, (int)app_context->last_ball_y, 3);
    }
}

static void hello_app_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;

    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static void timer_callback(void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;

    InputEvent event = {
        .type = 6, .key = 0}; //6 is just a number not used by anything else. sue me.
    furi_message_queue_put(event_queue, &event, 0);
}

int random_range(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

void beep() {
    if(furi_hal_speaker_acquire(500)) {
        float frequency = 100.0 + ((float)rand() / (float)(RAND_MAX / 450.0));
        furi_hal_speaker_start(frequency, 0.3);
        furi_delay_ms(50);
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }
}

int32_t

    flipper_hello_app(void* p) {
    UNUSED(p);

    InputEvent event;
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    struct app_context status;
    status.ball_x = (float)random_range(1, 126);
    status.ball_y = (float)random_range(1, 62);
    status.ball_speed_x = (float)random_range(-2, 2);
    status.ball_speed_y = (float)random_range(-2, 2);
    status.ghost_active = false;

    // Configure view port
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, hello_app_draw_callback, &status);
    view_port_input_callback_set(view_port, hello_app_input_callback, event_queue);

    // Register view port in GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Set timer
    FuriTimer* timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, event_queue);
    furi_timer_start(timer, furi_kernel_get_tick_frequency() / 16);

    while(1) {
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        //Handle input
        if(event.type != 6) {
            if(event.key == InputKeyBack) {
                break;
            }

            if(event.key == InputKeyUp) {
                if(status.ball_speed_y >= 0.0) {
                    status.ball_speed_y += 0.1;
                } else {
                    status.ball_speed_y -= 0.1;
                }
            }

            if(event.key == InputKeyDown) {
                if(status.ball_speed_y >= 0.0) {
                    status.ball_speed_y -= 0.1;
                } else {
                    status.ball_speed_y += 0.1;
                }
            }

            if(event.key == InputKeyRight) {
                if(status.ball_speed_x >= 0.0) {
                    status.ball_speed_x += 0.1;
                } else {
                    status.ball_speed_x -= 0.1;
                }
            }

            if(event.key == InputKeyLeft) {
                if(status.ball_speed_x >= 0.0) {
                    status.ball_speed_x -= 0.1;
                } else {
                    status.ball_speed_x += 0.1;
                }
            }

            if(event.key == InputKeyOk) {
                status.ghost_active = !status.ghost_active;
            }
        } else {
            //Calculate ball positioning
            status.last_ball_x = status.ball_x;
            status.last_ball_y = status.ball_y;
            status.ball_x += status.ball_speed_x;
            status.ball_y += status.ball_speed_y;

            //"Collision" detection
            if(status.ball_x >= 124.0) {
                status.ball_x = 124.0;
                status.ball_speed_x = -(status.ball_speed_x);
                beep();
            }

            if(status.ball_x <= 3.0) {
                status.ball_x = 3.0;
                status.ball_speed_x = -(status.ball_speed_x);
                beep();
            }

            if(status.ball_y >= 60.0) {
                status.ball_y = 60.0;
                status.ball_speed_y = -(status.ball_speed_y);
                beep();
            }

            if(status.ball_y <= 3.0) {
                status.ball_y = 3.0;
                status.ball_speed_y = -(status.ball_speed_y);
                beep();
            }

            view_port_update(view_port);
        }
    }

    furi_message_queue_free(event_queue);
    furi_timer_free(timer);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    return 0;
}