#pragma once
#include <bitset>
#include <memory>
#include <vector>
#include "BaseTypes.h"
#include "Cartridge.h"
#include "MemoryMap.h"
#include "Register.h"

namespace emu
{
    class Emulator
    {
    public:
        Emulator(std::unique_ptr<Cartridge> cartridge, int clockFrequency)
            : cartridge(std::move(cartridge)),
              screen(),
              af(),
              bc(),
              de(),
              hl(),
              programCounter(),
              stackPointer(),
              memory(),
              RAMBanks(),
              bankMode(),
              clockFrequency(clockFrequency),
              timerCounter(Emulator::CLOCK_SPEED / clockFrequency)
        {
            this->reset();
        }

        constexpr void reset()
        {
            this->initialize();
        }

        void update()
        {
            while (this->currentCycleCount < Emulator::MAX_CYCLES) {
                int cycles_passed = this->executeNextOpcode();
                this->currentCycleCount += cycles_passed;
                this->updateTimers(cycles_passed);
            }
        }

    private:
        constexpr void initialize()
        {
            this->programCounter = 0x100;

            this->af = {0x01B0};
            this->bc = {0x0013};
            this->de = {0x00D8};
            this->hl = {0x014D};

            this->stackPointer = {0xFFFE};

            this->memory[0xFF05] = 0x00;
            this->memory[0xFF06] = 0x00;
            this->memory[0xFF07] = 0x00;
            this->memory[0xFF10] = 0x80;
            this->memory[0xFF11] = 0xBF;
            this->memory[0xFF12] = 0xF3;
            this->memory[0xFF14] = 0xBF;
            this->memory[0xFF16] = 0x3F;
            this->memory[0xFF17] = 0x00;
            this->memory[0xFF19] = 0xBF;
            this->memory[0xFF1A] = 0x7F;
            this->memory[0xFF1B] = 0xFF;
            this->memory[0xFF1C] = 0x9F;
            this->memory[0xFF1E] = 0xBF;
            this->memory[0xFF20] = 0xFF;
            this->memory[0xFF21] = 0x00;
            this->memory[0xFF22] = 0x00;
            this->memory[0xFF23] = 0xBF;
            this->memory[0xFF24] = 0x77;
            this->memory[0xFF25] = 0xF3;
            this->memory[0xFF26] = 0xF1;
            this->memory[0xFF40] = 0x91;
            this->memory[0xFF42] = 0x00;
            this->memory[0xFF43] = 0x00;
            this->memory[0xFF45] = 0x00;
            this->memory[0xFF47] = 0xFC;
            this->memory[0xFF48] = 0xFF;
            this->memory[0xFF49] = 0xFF;
            this->memory[0xFF4A] = 0x00;
            this->memory[0xFF4B] = 0x00;
            this->memory[0xFFFF] = 0x00;

            this->bankMode = this->cartridge->detectBankMode();
        }

        void performUpdateLoop()
        {

        }

        void updateTimers(int cycles)
        {
            this->doDividerRegister(cycles);
            if (this->isClockEnabled()) {
                this->handleClock(cycles);
            }
        }

        void handleClock(int cycles)
        {
            this->timerCounter -= cycles;
            if (this->timerCounter <= 0) {
                this->setClockFrequency();
                if (this->readFromMemory(Emulator::TIMA) == 255) {
                    this->writeToMemory(Emulator::TIMA, this->readFromMemory(Emulator::TMA));
                    // Request interrupt
                }
                else {
                    this->writeToMemory(Emulator::TIMA,this->readFromMemory(Emulator::TIMA) + 1);
                }
            }
        }

        [[nodiscard]] emu::byte readFromMemory(const emu::word &address) const
        {
            if (mem::isSwitchableROMBank(address)) {
                return this->readFromROMBank(address);
            }
            else if (mem::isExternalRAM(address)) {
                return this->readFromRAMBank(address);
            }
            else {
                return this->performInternalMemoryRead(address);
            }
        }

        [[nodiscard]] emu::byte readFromROMBank(const emu::word &address) const
        {
            auto relativeAddress = address - mem::ROM_BANK_01_NN;
            auto translatedAddress = relativeAddress + this->currentROMBank * mem::ROM_BANK_01_NN;
            return this->performCartridgeMemoryRead(translatedAddress);
        }

        [[nodiscard]] emu::byte readFromRAMBank(const emu::word &address) const
        {
            auto relativeAddress = address - mem::EXTERNAL_RAM;
            auto translatedAddress = relativeAddress + this->currentRAMBank * Emulator::RAM_BANK_SIZE;
            return this->performRAMBankRead(translatedAddress);
        }

        [[nodiscard]] emu::byte performInternalMemoryRead(const emu::word &address) const
        {
            return this->memory[address];
        }

        [[nodiscard]] emu::byte performCartridgeMemoryRead(const emu::word &address) const
        {
            return this->cartridge->read(address);
        }

        [[nodiscard]] emu::byte performRAMBankRead(const emu::word &address) const
        {
            return this->RAMBanks[address];
        }

        void writeToMemory(const mem::addr &address, const emu::byte &data) // NOLINT(misc-no-recursion)
        {
            if (mem::isROM(address)) {
                // Don't write to ROM. Instead, handle banking
                this->handleBanking(address, data);
            }
            else if (mem::isExternalRAM(address)) {
                this->handleExternalRAMWrite(address, data);
            }
            else if (mem::isEcho(address)) {
                this->writeEcho(address, data);
            }
            else if (address == Emulator::DIVIDER_REGISTER) {
                // Trap the divider register
                this->memory[Emulator::DIVIDER_REGISTER] = 0;
            }
            else if (address == Emulator::TMC) {
                this->handleWriteToTMC(address, data);
            }
            else if (mem::isNotUsable(address)) {
                // Don't write there
            }
            else {
                this->performWriteToInternalMemory(address, data);
            }
        }

        void handleBanking(const mem::addr &address, const emu::byte &data)
        {
            if (mem::isRAMEnabling(address)) {
                this->handleRAMEnabling(address, data);
            }
            else if (mem::isROMBankChange(address)) {
                this->handleROMBankChange(data);
            }
            else if (mem::isROMOrRAMBankChange(address)) {
                this->handleROMOrRAMBankChange(data);
            }
            else if (mem::isROMRAMModeChange(address)) {
                this->changeROMRAMMode(data);
            }
        }

        void handleExternalRAMWrite(const mem::addr &address, const emu::byte &data)
        {
            if (this->RAMBankingEnabled) {
                this->writeToRAMBank(address, data);
            }
        }

        void handleRAMEnabling(const mem::addr &address, const emu::byte &data)
        {
            if (this->bankMode == BankMode::MBC1 || this->bankMode == BankMode::MBC2) {
                this->switchRAMBanking(address, data);
            }
        }

        void handleROMBankChange(const emu::byte &data)
        {
            if (this->bankMode == BankMode::MBC1 || this->bankMode == BankMode::MBC2) {
                this->changeLoROMBank(data);
            }
        }

        void handleROMOrRAMBankChange(const emu::byte &data)
        {
            if (this->bankMode == BankMode::MBC1) {
                if (this->ROMBankingEnabled) {
                    this->changeHiROMBank(data);
                }
                else {
                    this->changeRAMBank(data);
                }
            }
        }

        void handleWriteToTMC(const mem::addr &address, const emu::byte &data)
        {
            auto currentFrequency = this->getClockFrequency();
            this->memory[Emulator::TMC] = data;
            auto newFrequency = this->getClockFrequency();
            if (currentFrequency != newFrequency) {
                this->setClockFrequency();
            }
        }

        void switchRAMBanking(const mem::addr &address, const emu::byte &data)
        {
            if (this->bankMode == BankMode::MBC2) {
                std::bitset<16> address_bits(address);
                if (address_bits[4] == 1) {
                    return;
                }
            }
            emu::byte data_lower_nibble = data & 0xF;
            if (data_lower_nibble == 0xA) {
                this->RAMBankingEnabled = true;
            }
            else if (data_lower_nibble == 0x0) {
                this->RAMBankingEnabled = false;
            }
        }

        void changeLoROMBank(const emu::byte &data)
        {
            if (this->bankMode == BankMode::MBC2) {
                this->currentROMBank = data & 0xF;
                if (this->currentROMBank == 0) {
                    this->currentROMBank++;
                }
            }
            else {
                emu::byte lower_5_bits = data & 31;
                this->currentROMBank &= 224;
                this->currentROMBank |= lower_5_bits;
                if (this->currentROMBank == 0) {
                    this->currentROMBank++;
                }
            }
        }

        void changeHiROMBank(const emu::byte &data)
        {
            this->currentROMBank &= 31;
            auto data_tmp = data & 224;
            this->currentROMBank |= data_tmp;
            if (this->currentROMBank == 0) {
                this->currentROMBank++;
            }
        }

        void changeRAMBank(const emu::byte &data)
        {
            this->currentRAMBank = data & 0x3;
        }

        void changeROMRAMMode(const emu::byte &data)
        {
            emu::byte data_tmp = data & 0x1;
            this->ROMBankingEnabled = data_tmp == 0;
            if (this->ROMBankingEnabled) {
                this->currentRAMBank = 0;
            }
        }

        void writeToRAMBank(const mem::addr &address, const emu::byte &data)
        {
            auto relativeAddress = address - mem::EXTERNAL_RAM;
            auto translatedAddress = relativeAddress + (this->currentRAMBank * Emulator::RAM_BANK_SIZE);
            this->RAMBanks[translatedAddress] = data;
        }

        void writeEcho(const mem::addr &address, const emu::byte &data) // NOLINT(misc-no-recursion)
        {
            this->performWriteToInternalMemory(address, data);

            auto echoedAddress = address - Emulator::ECHO_RAM_ADDRESS_SUBTRACT;
            this->writeToMemory(echoedAddress, data);
        }

        void performWriteToInternalMemory(const mem::addr &address, const emu::byte &data)
        {
            this->memory[address] = data;
        }

        int executeNextOpcode()
        {
            auto opcode = this->cartridge->read(this->programCounter);
            return 0;
        }

        [[nodiscard]] bool isClockEnabled() const
        {
            auto tmc_value = this->readFromMemory(Emulator::TMC);
            std::bitset<3> tcm_bitset(tmc_value);
            return tcm_bitset[2] == 1;
        }

        [[nodiscard]] emu::byte getClockFrequency() const
        {
            auto tmc_value = this->readFromMemory(Emulator::TMC);
            return tmc_value & 0x3;
        }

        void setClockFrequency()
        {
            auto frequency = this->getClockFrequency();
            switch (frequency) {
                case 0: // 4096
                    this->timerCounter = 1024;
                    break;
                case 1: // 262144
                    this->timerCounter = 16;
                    break;
                case 2: //65536
                    this->timerCounter = 64;
                    break;
                case 3: //16382
                    this->timerCounter = 256;
                    break;
                default: // Shouldn't happen
                    this->timerCounter = 1024;
                    break;
            }
        }

        void doDividerRegister(int cycles)
        {
            this->dividerCounter += cycles;
            if (this->dividerCounter >= 255)
            {
                this->dividerCounter = 0;
                this->memory[Emulator::DIVIDER_REGISTER]++;
            }
        }

        std::unique_ptr<emu::Cartridge> cartridge;

        emu::byte screen[144][160][3];

        emu::Register af;
        emu::Register bc;
        emu::Register de;
        emu::Register hl;

        emu::word programCounter;
        emu::Register stackPointer; // Emulated as a register because some opcodes might use hi and lo bytes separately

        BankMode bankMode;

        bool ROMBankingEnabled = true;
        emu::byte currentROMBank = 1;

        bool RAMBankingEnabled = false;
        emu::byte currentRAMBank = 0;
        emu::byte RAMBanks[0x8000];

        emu::byte memory[0x10000];

        int currentCycleCount = 0;
        int timerCounter;
        int clockFrequency;

        int dividerCounter = 0;

        static constexpr int CLOCK_SPEED = 4194304;
        static constexpr int MAX_CYCLES = 69905;

        static constexpr mem::addr DIVIDER_REGISTER = 0xFF04;

        static constexpr mem::addr TIMA = 0xFF05;   // Timer address
        static constexpr mem::addr TMA = 0xFF06;    // Timer modulator address (stores address to value to set timer to
        // after overflow)
        static constexpr mem::addr TMC = 0xFF07;    // Timer controller address

        static constexpr mem::addr ECHO_RAM_ADDRESS_SUBTRACT = 0x2000;
        static constexpr mem::addr RAM_BANK_SIZE = 0x2000;
    };
}
