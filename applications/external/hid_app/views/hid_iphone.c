#include "hid_Iphone.h"
#include "../hid.h"
#include <gui/elements.h>

#include "hid_icons.h"

#define TAG "HidIphone"

struct HidIphone {
    View* view;
    Hid* hid;
};

typedef struct {
    bool left_pressed;
    bool up_pressed;
    bool right_pressed;
    bool down_pressed;
    bool ok_pressed;
    bool connected;
    bool is_cursor_set;
    HidTransport transport;
} HidIphoneModel;

static void hid_Iphone_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    HidIphoneModel* model = context;

    // Header
    if(model->transport == HidTransportBle) {
        if(model->connected) {
            canvas_draw_icon(canvas, 0, 0, &I_Ble_connected_15x15);
        } else {
            canvas_draw_icon(canvas, 0, 0, &I_Ble_disconnected_15x15);
        }
    }

    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(canvas, 17, 3, AlignLeft, AlignTop, "Iphone");
    canvas_set_font(canvas, FontSecondary);

    // Keypad circles
    canvas_draw_icon(canvas, 76, 8, &I_Circles_47x47);

    // Up
    if(model->up_pressed) {
        canvas_set_bitmap_mode(canvas, 1);
        canvas_draw_icon(canvas, 93, 9, &I_Pressed_Button_13x13);
        canvas_set_bitmap_mode(canvas, 0);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_icon(canvas, 96, 11, &I_Arr_up_7x9);
    canvas_set_color(canvas, ColorBlack);

    // Down
    if(model->down_pressed) {
        canvas_set_bitmap_mode(canvas, 1);
        canvas_draw_icon(canvas, 93, 41, &I_Pressed_Button_13x13);
        canvas_set_bitmap_mode(canvas, 0);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_icon(canvas, 96, 44, &I_Arr_dwn_7x9);
    canvas_set_color(canvas, ColorBlack);

    // Left
    if(model->left_pressed) {
        canvas_set_bitmap_mode(canvas, 1);
        canvas_draw_icon(canvas, 77, 25, &I_Pressed_Button_13x13);
        canvas_set_bitmap_mode(canvas, 0);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_icon(canvas, 81, 29, &I_Voldwn_6x6);
    canvas_set_color(canvas, ColorBlack);

    // Right
    if(model->right_pressed) {
        canvas_set_bitmap_mode(canvas, 1);
        canvas_draw_icon(canvas, 109, 25, &I_Pressed_Button_13x13);
        canvas_set_bitmap_mode(canvas, 0);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_icon(canvas, 111, 29, &I_Volup_8x6);
    canvas_set_color(canvas, ColorBlack);

    // Ok
    if(model->ok_pressed) {
        canvas_draw_icon(canvas, 91, 23, &I_Like_pressed_17x17);
    } else {
        canvas_draw_icon(canvas, 94, 27, &I_Like_def_11x9);
    }
    // Exit
    canvas_draw_icon(canvas, 0, 54, &I_Pin_back_arrow_10x8);
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 13, 62, AlignLeft, AlignBottom, "Hold to exit");
}

static void hid_Iphone_reset_cursor(HidIphone* hid_Iphone) {
    // Set cursor to the phone's left up corner
    // Delays to guarantee one packet per connection interval
    for(size_t i = 0; i < 8; i++) {
        hid_hal_mouse_move(hid_Iphone->hid, -127, -127);
        furi_delay_ms(50);
    }
    // Move cursor from the corner
    hid_hal_mouse_move(hid_Iphone->hid, 20, 120);
    furi_delay_ms(50);
}

static void
    hid_Iphone_process_press(HidIphone* hid_Iphone, HidIphoneModel* model, InputEvent* event) {
    if(event->key == InputKeyUp) {
        model->up_pressed = true;
    } else if(event->key == InputKeyDown) {
        model->down_pressed = true;
    } else if(event->key == InputKeyLeft) {
        model->left_pressed = true;
        hid_hal_consumer_key_press(hid_Iphone->hid, (1 << 20) & HID_KEYBOARD_H);
    } else if(event->key == InputKeyRight) {
        model->right_pressed = true;
        hid_hal_consumer_key_press(hid_Iphone->hid, HID_CONSUMER_VOLUME_INCREMENT);
    } else if(event->key == InputKeyOk) {
        model->ok_pressed = true;
    }
}

static void
    hid_Iphone_process_release(HidIphone* hid_Iphone, HidIphoneModel* model, InputEvent* event) {
    if(event->key == InputKeyUp) {
        model->up_pressed = false;
    } else if(event->key == InputKeyDown) {
        model->down_pressed = false;
    } else if(event->key == InputKeyLeft) {
        model->left_pressed = false;
        hid_hal_consumer_key_press(hid_Iphone->hid, (1 << 20) & HID_KEYBOARD_H);
    } else if(event->key == InputKeyRight) {
        model->right_pressed = false;
        hid_hal_consumer_key_release(hid_Iphone->hid, HID_CONSUMER_VOLUME_INCREMENT);
    } else if(event->key == InputKeyOk) {
        model->ok_pressed = false;
    }
}

static bool hid_Iphone_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    HidIphone* hid_Iphone = context;
    bool consumed = false;

    with_view_model(
        hid_Iphone->view,
        HidIphoneModel * model,
        {
            if(event->type == InputTypePress) {
                hid_Iphone_process_press(hid_Iphone, model, event);
                if(model->connected && !model->is_cursor_set) {
                    hid_Iphone_reset_cursor(hid_Iphone);
                    model->is_cursor_set = true;
                }
                consumed = true;
            } else if(event->type == InputTypeRelease) {
                hid_Iphone_process_release(hid_Iphone, model, event);
                consumed = true;
            } else if(event->type == InputTypeShort) {
                if(event->key == InputKeyOk) {
                    hid_hal_mouse_press(hid_Iphone->hid, HID_MOUSE_BTN_LEFT);
                    furi_delay_ms(50);
                    hid_hal_mouse_release(hid_Iphone->hid, HID_MOUSE_BTN_LEFT);
                    furi_delay_ms(50);
                    hid_hal_mouse_press(hid_Iphone->hid, HID_MOUSE_BTN_LEFT);
                    furi_delay_ms(50);
                    hid_hal_mouse_release(hid_Iphone->hid, HID_MOUSE_BTN_LEFT);
                    consumed = true;
                } else if(event->key == InputKeyUp) {
                    // Emulate up swipe
                    hid_hal_mouse_scroll(hid_Iphone->hid, -6);
                    hid_hal_mouse_scroll(hid_Iphone->hid, -12);
                    hid_hal_mouse_scroll(hid_Iphone->hid, -19);
                    hid_hal_mouse_scroll(hid_Iphone->hid, -12);
                    hid_hal_mouse_scroll(hid_Iphone->hid, -6);
                    consumed = true;
                } else if(event->key == InputKeyDown) {
                    // Emulate down swipe
                    hid_hal_mouse_scroll(hid_Iphone->hid, 6);
                    hid_hal_mouse_scroll(hid_Iphone->hid, 12);
                    hid_hal_mouse_scroll(hid_Iphone->hid, 19);
                    hid_hal_mouse_scroll(hid_Iphone->hid, 12);
                    hid_hal_mouse_scroll(hid_Iphone->hid, 6);
                    consumed = true;
                } else if(event->key == InputKeyBack) {
                    hid_hal_consumer_key_release_all(hid_Iphone->hid);
                    consumed = true;
                }
            } else if(event->type == InputTypeLong) {
                if(event->key == InputKeyBack) {
                    hid_hal_consumer_key_release_all(hid_Iphone->hid);
                    model->is_cursor_set = false;
                    consumed = false;
                }
            }
        },
        true);

    return consumed;
}

HidIphone* hid_Iphone_alloc(Hid* bt_hid) {
    HidIphone* hid_Iphone = malloc(sizeof(HidIphone));
    hid_Iphone->hid = bt_hid;
    hid_Iphone->view = view_alloc();
    view_set_context(hid_Iphone->view, hid_Iphone);
    view_allocate_model(hid_Iphone->view, ViewModelTypeLocking, sizeof(HidIphoneModel));
    view_set_draw_callback(hid_Iphone->view, hid_Iphone_draw_callback);
    view_set_input_callback(hid_Iphone->view, hid_Iphone_input_callback);

    with_view_model(
        hid_Iphone->view, HidIphoneModel * model, { model->transport = bt_hid->transport; }, true);

    return hid_Iphone;
}

void hid_Iphone_free(HidIphone* hid_Iphone) {
    furi_assert(hid_Iphone);
    view_free(hid_Iphone->view);
    free(hid_Iphone);
}

View* hid_Iphone_get_view(HidIphone* hid_Iphone) {
    furi_assert(hid_Iphone);
    return hid_Iphone->view;
}

void hid_Iphone_set_connected_status(HidIphone* hid_Iphone, bool connected) {
    furi_assert(hid_Iphone);
    with_view_model(
        hid_Iphone->view,
        HidIphoneModel * model,
        {
            model->connected = connected;
            model->is_cursor_set = false;
        },
        true);
}
