#pragma once
#include "BaseTypes.h"

namespace emu::mem
{
    using addr = emu::word;

    constexpr addr ROM_BANK_00 = 0x0000;
    constexpr addr ROM_BANK_01_NN = 0x4000;
    constexpr addr VRAM = 0x8000;
    constexpr addr EXTERNAL_RAM = 0xA000;
    constexpr addr WRAM_0 = 0xC000;
    constexpr addr WRAM_1 = 0xD000;
    constexpr addr ECHO = 0xE000;
    constexpr addr OAM = 0xFE00;
    constexpr addr NOT_USABLE = 0xFEA0;
    constexpr addr IO = 0xFF00;
    constexpr addr HRAM = 0xFF80;
    constexpr addr INTERRUPT_ENABLE_REGISTER = 0xFFFF;

    static constexpr addr DIVIDER_REGISTER = 0xFF04;

    static constexpr mem::addr TIMA = 0xFF05;   // Timer address
    static constexpr mem::addr TMA = 0xFF06;    // Timer modulator address (stores address to value to set timer to
    // after overflow)
    static constexpr mem::addr TMC = 0xFF07;    // Timer controller address

    constexpr addr BANKING_ENABLE_RAM = 0x0;
    constexpr addr BANKING_CHANGE_ROM_BANK = 0x2000;
    constexpr addr BANKING_CHANGE_ROM_OR_RAM_BANK = 0x4000;
    constexpr addr BANKING_CHANGE_ROM_RAM_MODE = 0x6000;

    constexpr bool isROM(const addr &address)
    {
        return address < ROM_BANK_01_NN;
    }

    constexpr bool isExternalRAM(const addr &address)
    {
        return address >= EXTERNAL_RAM && address < WRAM_0;
    }

    constexpr bool isEcho(const addr &address)
    {
        return address >= ECHO && address < OAM;
    }

    constexpr bool isNotUsable(const addr &address)
    {
        return address >= NOT_USABLE && address < IO;
    }

    constexpr bool isSwitchableROMBank(const addr &address)
    {
        return address >= ROM_BANK_01_NN && address < VRAM;
    }

    constexpr bool isRAMEnabling(const addr &address)
    {
        return address >= BANKING_ENABLE_RAM && address < BANKING_CHANGE_ROM_BANK;
    }

    constexpr bool isROMBankChange(const addr &address)
    {
        return address >= BANKING_CHANGE_ROM_BANK && address < BANKING_CHANGE_ROM_OR_RAM_BANK;
    }

    constexpr bool isROMOrRAMBankChange(const addr &address)
    {
        return address >= BANKING_CHANGE_ROM_OR_RAM_BANK && address < BANKING_CHANGE_ROM_RAM_MODE;
    }

    constexpr bool isROMRAMModeChange(const addr &address)
    {
        return address >= BANKING_CHANGE_ROM_RAM_MODE && address < VRAM;
    }
}