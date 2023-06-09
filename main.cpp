#include <memory>

#include "Cartridge.h"
#include "Emulator.h"

int main() {
    std::unique_ptr<emu::Cartridge> cartridge;
    cartridge->load("pokemon-red.gb");
    auto clockFrequency = 4096;
    auto emulator = std::make_unique<emu::Emulator>(std::move(cartridge), clockFrequency);
    emulator->reset();

    return 0;
}
