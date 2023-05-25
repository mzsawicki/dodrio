#pragma once
#include <memory>
#include <string>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include "BaseTypes.h"


namespace emu
{
    using file_stream = std::basic_fstream<emu::byte>;

    enum BankMode
    {
        MBC1,
        MBC2,
        NONE
    };

    class Cartridge
    {
    public:
        constexpr Cartridge() = default;
        void load(const std::string &file_name)
        {
            if (!std::filesystem::exists(file_name)) {
                throw std::runtime_error(FILE_NOT_FOUND_MESSAGE);
            }

            file_stream stream(file_name, std::ios::binary|std::ios::in);
            if (!stream.is_open()) {
                throw std::runtime_error(FILE_CANNOT_BE_ACCESSED);
            }
            auto pos = stream.tellg();
            stream.seekg(0, std::ios::beg);
            stream.read(this->memory, pos);
        }

        [[nodiscard]] BankMode detectBankMode() const
        {
            auto bankModeIndicator = this->read(Cartridge::BANK_MODE_ADDRESS);
            switch (bankModeIndicator) {
                case 0:
                    return BankMode::NONE;
                case 1:
                case 2:
                case 3:
                    return BankMode::MBC1;
                case 5:
                case 6:
                    return BankMode::MBC2;
                default:
                    return BankMode::NONE; // Although this shouldn't happen
            }
        }

        [[nodiscard]] emu::byte detectRAMBanksCount() const
        {
            return this->read(Cartridge::RAM_BANKS_COUNT_ADDRESS);
        }

        [[nodiscard]] emu::byte read(emu::word address) const
        {
            return this->memory[address];
        }

    private:
        constexpr static int MEMORY_SIZE = 0x200000;
        constexpr static char FILE_NOT_FOUND_MESSAGE[] = "Game file not found.";
        constexpr static char FILE_CANNOT_BE_ACCESSED[] = "Game file cannot be accessed.";

        static constexpr emu::word BANK_MODE_ADDRESS = 0x147;
        static constexpr emu::word RAM_BANKS_COUNT_ADDRESS = 0x148;

        emu::byte memory[MEMORY_SIZE] = { 0 };
    };
}