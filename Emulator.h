#pragma once
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
        explicit Emulator(std::unique_ptr<Cartridge> cartridge)
            : cartridge(std::move(cartridge)),
              screen(),
              af(),
              bc(),
              de(),
              hl(),
              programCounter(),
              stackPointer(),
              memory(),
              RAMBanks()
        {
            this->reset();
        }

        constexpr void reset()
        {
            this->initialize();
        }

        void update()
        {
            while (this->currentCycleCount < MAX_CYCLES) {
                break;
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
        }

        void performUpdateLoop()
        {

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
                this->handleROMBankChange(address, data);
            }
            else if (mem::isROMOrRAMBankChange(address)) {
                this->handleROMOrRAMBankChange(address, data);
            }
            else if (mem::isROMRAMModeChange(address)) {
                this->changeROMRAMMode(address, data);
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
            auto bankMode = this->detectBankMode();
            if (bankMode == BankMode::MBC1 || bankMode == BankMode::MBC2) {
                this->enableRAMBank(address, data);
            }
        }

        void handleROMBankChange(const mem::addr &address, const emu::byte &data)
        {
            auto bankMode = this->detectBankMode();
            if (bankMode == BankMode::MBC1 || bankMode == BankMode::MBC2) {
                this->changeLoROMBank(address, data);
            }
        }

        void handleROMOrRAMBankChange(const mem::addr &address, const emu::byte &data)
        {
            auto bankMode = this->detectBankMode();
            if (bankMode == BankMode::MBC1) {
                if (this->ROMBankingEnabled) {
                    this->changeHiROMBank(address, data);
                }
                else {
                    this->changeRAMBank(address, data);
                }
            }
        }

        void enableRAMBank(const mem::addr &address, const emu::byte &data)
        {

        }

        void changeLoROMBank(const mem::addr &address, const emu::byte &data)
        {

        }

        void changeHiROMBank(const mem::addr &address, const emu::byte &data)
        {

        }

        void changeRAMBank(const mem::addr &address, const emu::byte &data)
        {

        }

        void changeROMRAMMode(const mem::addr &address, const emu::byte &data)
        {

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

        void executeNextOpcode()
        {
            auto opcode = this->cartridge->read(this->programCounter);
        }

        [[nodiscard]] BankMode detectBankMode() const
        {
            return this->cartridge->detectBankMode();
        }

        std::unique_ptr<emu::Cartridge> cartridge;

        emu::byte screen[144][160][3];

        emu::Register af;
        emu::Register bc;
        emu::Register de;
        emu::Register hl;

        emu::word programCounter;
        emu::Register stackPointer; // Emulated as a register because some opcodes might use hi and lo bytes separately

        bool ROMBankingEnabled = true;
        emu::byte currentROMBank = 1;

        bool RAMBankingEnabled = false;
        emu::byte currentRAMBank = 0;
        emu::byte RAMBanks[0x8000];

        emu::byte memory[0x10000];

        int currentCycleCount = 0;

        static constexpr int MAX_CYCLES = 69905;

        static constexpr mem::addr ECHO_RAM_ADDRESS_SUBTRACT = 0x2000;
        static constexpr mem::addr RAM_BANK_SIZE = 0x2000;
    };
}
