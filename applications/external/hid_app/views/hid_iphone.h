#pragma once

#include <gui/view.h>

typedef struct Hid Hid;
typedef struct HidIphone HidIphone;

HidIphone* hid_iphone_alloc(Hid* bt_hid);

void hid_iphone_free(HidIphone* hid_iphone);

View* hid_iphone_get_view(HidIphone* hid_iphone);

void hid_iphone_set_connected_status(HidIphone* hid_iphone, bool connected);
