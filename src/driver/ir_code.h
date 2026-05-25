//  infrared code value type
//
//  IrCode is a compact 32-bit token shared by receive, events, and transmit.
//
//  reserved raw values:
//  - 0x00000000: invalid/no code, transmitters ignore it
//  - 0xffffffff: NEC repeat, transmitters send a NEC repeat frame
//  - all other values: 32-bit data frames
//
//  NEC helpers pack and decode addr, ~addr, cmd, ~cmd byte order.
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/types.h"

using lite::u8;
using lite::u32;

//==============================================================================
class IrCode {
//------------------------------------------------------------------------------
public:
    struct Nec {
        u8 addr;
        u8 cmd;
    };

    static constexpr u32 k_invalid_raw = 0x00000000u;
    static constexpr u32 k_repeat_raw = 0xffffffffu;

    constexpr IrCode() noexcept = default;

    static constexpr IrCode invalid() noexcept {
        return IrCode(k_invalid_raw);
    }

    static constexpr IrCode repeat() noexcept {
        return IrCode(k_repeat_raw);
    }

    static constexpr IrCode from_raw(u32 raw) noexcept {
        return IrCode(raw);
    }

    static constexpr IrCode from_nec(u8 addr, u8 cmd) noexcept {
        const u8 addr_inv = static_cast<u8>(~addr);
        const u8 cmd_inv = static_cast<u8>(~cmd);

        return IrCode(
            static_cast<u32>(addr)
            | (static_cast<u32>(addr_inv) << 8)
            | (static_cast<u32>(cmd) << 16)
            | (static_cast<u32>(cmd_inv) << 24)
        );
    }

    constexpr u32 raw() const noexcept {
        return static_cast<u32>(raw_[0])
            | (static_cast<u32>(raw_[1]) << 8)
            | (static_cast<u32>(raw_[2]) << 16)
            | (static_cast<u32>(raw_[3]) << 24);
    }

    constexpr bool is_invalid() const noexcept {
        return raw() == k_invalid_raw;
    }

    constexpr bool is_repeat() const noexcept {
        return raw() == k_repeat_raw;
    }

    constexpr bool is_valid() const noexcept {
        return !is_invalid();
    }

    constexpr bool is_data() const noexcept {
        return is_valid() && !is_repeat();
    }

    constexpr bool is_nec() const noexcept {
        Nec nec;
        return decode_nec(nec);
    }

    constexpr bool decode_nec(Nec& nec) const noexcept {
        if (!is_data()) {
            return false;
        }

        const u8 addr = raw_[0];
        const u8 addr_inv = raw_[1];
        const u8 cmd = raw_[2];
        const u8 cmd_inv = raw_[3];

        if (static_cast<u8>(~addr) != addr_inv) {
            return false;
        }

        if (static_cast<u8>(~cmd) != cmd_inv) {
            return false;
        }

        nec = { .addr = addr, .cmd = cmd };
        return true;
    }

//------------------------------------------------------------------------------
private:
    explicit constexpr IrCode(u32 raw) noexcept
        : raw_ {
            static_cast<u8>(raw),
            static_cast<u8>(raw >> 8),
            static_cast<u8>(raw >> 16),
            static_cast<u8>(raw >> 24)
        } {}

    u8 raw_[4] {};
};

static_assert(sizeof(IrCode) == sizeof(u32), "unexpected size of IrCode");
static_assert(alignof(IrCode) == alignof(u8), "unexpected alignment of IrCode");
