#pragma once

#include "constants/event_meta_const.hpp"

struct EventSlot {
    char code;
    char name[eventmeta::NAME_LEN];
};

struct EventMeta {
    unsigned int slotCount;
    EventSlot slots[eventmeta::SLOT_COUNT];
};
