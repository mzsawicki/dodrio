#pragma once
#include "BaseTypes.h"

namespace emu
{
    union Register
    {
        emu::word word;
        struct {
            emu::byte hi;
            emu::byte lo;
        };
    };
}