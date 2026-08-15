#define XBOX_IMPL
#include "xbox.h"

void xbox::LED::set(const std::array<Color, 4> &seq) {
    HalWriteSMBusValue(0x20, SMC_REG_LEDMODE, FALSE, MODE_MANUAL);
    ULONG colorScheme =
        ((seq[0] & 0x11) << 3) |
        ((seq[1] & 0x11) << 2) |
        ((seq[2] & 0x11) << 1) |
        ((seq[3] & 0x11) << 0);
    HalWriteSMBusValue(0x20, SMC_REG_LEDSEQ, FALSE, colorScheme);
}

void xbox::LED::reset() {
    HalWriteSMBusValue(0x20, SMC_REG_LEDMODE, FALSE, MODE_AUTO);
}