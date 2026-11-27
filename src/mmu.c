#include "mmu.h"

#include <string.h>

MMU *mmu_init(void)
{
    MMU *mmu = calloc(1, sizeof(MMU));
    if (mmu == NULL) {
        return NULL;
    }
    mmu->cartridge.rom_bank = 1u;
    mmu->action_buttons = 0x0Fu;
    mmu->dpad_buttons = 0x0Fu;
    return mmu;
}

void mmu_destroy(MMU *mmu)
{
    if (mmu == NULL) {
        return;
    }
    free(mmu->cartridge.rom);
    free(mmu->cartridge.ram);
    memset(&mmu->cartridge, 0, sizeof(mmu->cartridge));
    free(mmu);
}

static size_t cartridge_ram_size(uint8_t code)
{
    switch (code) {
    case 0x01: return 2048u;
    case 0x02: return 8192u;
    case 0x03: return 32768u;
    case 0x04: return 131072u;
    case 0x05: return 65536u;
    default: return 0u;
    }
}

static CartridgeType cartridge_type(uint8_t code)
{
    if (code == 0x05 || code == 0x06) return CARTRIDGE_MBC2;
    if (code == 0x00) return CARTRIDGE_ROM_ONLY;
    if (code >= 0x01 && code <= 0x03) return CARTRIDGE_MBC1;
    if (code >= 0x0F && code <= 0x13) return CARTRIDGE_MBC3;
    if (code >= 0x19 && code <= 0x1E) return CARTRIDGE_MBC5;
    if (code == 0x0B || code == 0x0C || code == 0x0D) return CARTRIDGE_MMM01;
    if (code == 0x20) return CARTRIDGE_MBC6;
    if (code == 0x22) return CARTRIDGE_MBC7;
    return CARTRIDGE_UNKNOWN;
}

static void build_save_path(MMU *mmu, const char *rom_name)
{
    size_t length = strlen(rom_name);
    size_t copy_length = length;
    const char *dot = strrchr(rom_name, '.');

    if (dot != NULL && dot > rom_name) {
        copy_length = (size_t)(dot - rom_name);
    }
    if (copy_length >= sizeof(mmu->cartridge.save_path)) {
        copy_length = sizeof(mmu->cartridge.save_path) - 1u;
    }
    memcpy(mmu->cartridge.save_path, rom_name, copy_length);
    memcpy(mmu->cartridge.save_path + copy_length, ".sav",
           strlen(".sav") + 1u);
}

bool mmu_load_rom(MMU *mmu, const char *rom_name)
{
    FILE *fp;
    long rom_size;
    Cartridge *cart;

    if (mmu == NULL || rom_name == NULL) return false;
    fp = fopen(rom_name, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open ROM file: %s\n", rom_name);
        return false;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return false;
    }
    rom_size = ftell(fp);
    if (rom_size < 1 || (unsigned long)rom_size > CARTRIDGE_ROM_MAX) {
        fprintf(stderr, "Invalid ROM size: %ld bytes\n", rom_size);
        fclose(fp);
        return false;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return false;
    }

    free(mmu->cartridge.rom);
    free(mmu->cartridge.ram);
    memset(&mmu->cartridge, 0, sizeof(mmu->cartridge));
    cart = &mmu->cartridge;
    cart->rom = malloc((size_t)rom_size);
    if (cart->rom == NULL) {
        fclose(fp);
        return false;
    }
    if (fread(cart->rom, 1, (size_t)rom_size, fp) != (size_t)rom_size) {
        fprintf(stderr, "Failed to read ROM: %s\n", rom_name);
        free(cart->rom);
        cart->rom = NULL;
        fclose(fp);
        return false;
    }
    fclose(fp);

    cart->rom_size = (size_t)rom_size;
    if ((size_t)rom_size >= 0x150u) {
        cart->type = cartridge_type(cart->rom[0x147u]);
        if (cart->type == CARTRIDGE_MBC2) {
            cart->ram_size = 512u;
        } else if (cart->type == CARTRIDGE_MBC6) {
            cart->ram_size = 32768u;
        } else if (cart->type == CARTRIDGE_MBC7) {
            cart->ram_size = 256u;
        } else {
            cart->ram_size = cartridge_ram_size(cart->rom[0x149u]);
        }
    }
    if (cart->ram_size > 0u) {
        cart->ram = calloc(1, cart->ram_size);
        if (cart->ram == NULL) return false;
    }
    cart->rom_bank = 1u;
    cart->ram_bank = 0u;
    build_save_path(mmu, rom_name);

    memcpy(mmu->memory, cart->rom, cart->rom_size < 0x8000u ? cart->rom_size : 0x8000u);
    mmu_load_battery(mmu);
    return true;
}

bool mmu_save_battery(MMU *mmu)
{
    FILE *fp;
    Cartridge *cart;

    if (mmu == NULL || mmu->cartridge.ram == NULL) return true;
    cart = &mmu->cartridge;
    fp = fopen(cart->save_path, "wb");
    if (fp == NULL) return false;
    if (fwrite(cart->ram, 1, cart->ram_size, fp) != cart->ram_size) {
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

bool mmu_load_battery(MMU *mmu)
{
    FILE *fp;
    Cartridge *cart;
    long size;

    if (mmu == NULL || mmu->cartridge.ram == NULL) return true;
    cart = &mmu->cartridge;
    fp = fopen(cart->save_path, "rb");
    if (fp == NULL) return true;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return false;
    }
    size = ftell(fp);
    if (size >= 0 && (size_t)size <= cart->ram_size) {
        rewind(fp);
        fread(cart->ram, 1, (size_t)size, fp);
    }
    fclose(fp);
    return true;
}

const char *mmu_cartridge_type_name(const MMU *mmu)
{
    if (mmu == NULL) return "Invalid";
    switch (mmu->cartridge.type) {
    case CARTRIDGE_ROM_ONLY: return "ROM";
    case CARTRIDGE_MBC1: return "MBC1";
    case CARTRIDGE_MBC2: return "MBC2";
    case CARTRIDGE_MBC3: return "MBC3";
    case CARTRIDGE_MBC5: return "MBC5";
    case CARTRIDGE_MMM01: return "MMM01";
    case CARTRIDGE_MBC6: return "MBC6";
    case CARTRIDGE_MBC7: return "MBC7";
    default: return "Unknown";
    }
}

static uint8_t cartridge_rom_read(const Cartridge *cart, uint16_t addr)
{
    size_t index;
    size_t bank;
    if (cart->rom == NULL || cart->rom_size == 0u) return 0xFFu;

    if (addr < 0x4000u) {
        bank = 0u;
    } else {
        bank = cart->rom_bank;
        if (bank == 0u) bank = 1u;
    }
    index = bank * 0x4000u + (addr & 0x3FFFu);
    if (index >= cart->rom_size) index %= cart->rom_size;
    return cart->rom[index];
}

static uint8_t cartridge_ram_read(const Cartridge *cart, uint16_t addr)
{
    size_t index;
    if (!cart->ram_enabled) return 0xFFu;
    if (cart->type == CARTRIDGE_MBC3 && cart->ram_bank >= 0x08u && cart->ram_bank <= 0x0Cu) {
        return cart->rtc[cart->ram_bank - 0x08u];
    }
    if (cart->ram == NULL) return 0xFFu;
    if (cart->type == CARTRIDGE_MBC2) {
        return cart->ram[addr & 0x1FFu] | 0xF0u;
    }
    index = (size_t)cart->ram_bank * 0x2000u + (addr - 0xA000u);
    if (index >= cart->ram_size) return 0xFFu;
    return cart->ram[index];
}

static void cartridge_ram_write(Cartridge *cart, uint16_t addr, uint8_t val)
{
    size_t index;
    if (!cart->ram_enabled) return;
    if (cart->type == CARTRIDGE_MBC3 && cart->ram_bank >= 0x08u && cart->ram_bank <= 0x0Cu) {
        cart->rtc[cart->ram_bank - 0x08u] = val;
        return;
    }
    if (cart->ram == NULL) return;
    if (cart->type == CARTRIDGE_MBC2) {
        cart->ram[addr & 0x1FFu] = val & 0x0Fu;
        return;
    }
    index = (size_t)cart->ram_bank * 0x2000u + (addr - 0xA000u);
    if (index >= cart->ram_size) return;
    cart->ram[index] = val;
}

uint8_t mmu_read_byte(MMU *mmu, uint16_t addr)
{
    if (mmu == NULL) return 0xFFu;
    if (mmu->cartridge.rom == NULL) {
        if (addr == 0xFF00u) {
            uint8_t select = mmu->memory[0xFF00u] & 0x30u;
            uint8_t result = 0xC0u | select;
            if (select == 0x20u) result |= mmu->action_buttons;
            else if (select == 0x10u) result |= mmu->dpad_buttons;
            else if (select == 0x30u) result |= mmu->action_buttons & mmu->dpad_buttons;
            else result |= 0x0Fu;
            return result;
        }
        if (addr >= 0xE000u && addr < 0xFE00u) {
            return mmu->memory[addr - 0x2000u];
        }
        return mmu->memory[addr];
    }
    if (addr < 0x8000u) return cartridge_rom_read(&mmu->cartridge, addr);
    if (addr >= 0xA000u && addr < 0xC000u) return cartridge_ram_read(&mmu->cartridge, addr);
    if (addr >= 0xE000u && addr < 0xFE00u) return mmu->memory[addr - 0x2000u];
    if (addr >= 0xFEA0u && addr < 0xFF00u) return 0xFFu;
    return mmu->memory[addr];
}

void mmu_write_byte(MMU *mmu, uint16_t addr, uint8_t val)
{
    Cartridge *cart;
    if (mmu == NULL) return;
    cart = &mmu->cartridge;
    if (addr == 0xFF00u) {
        mmu->memory[addr] = val & 0x30u;
        return;
    }
    if (addr == 0xFF04u) {
        mmu->memory[addr] = 0u;
        return;
    }
    if (addr == 0xFF46u) {
        mmu->dma_source = val;
        mmu->dma_active = true;
        mmu->dma_remaining = 160u;
        mmu->memory[addr] = val;
        return;
    }
    if (cart->rom == NULL) {
        if (addr == 0xFF04u) {
            mmu->memory[addr] = 0u;
            return;
        }
        if (addr == 0xFF46u) {
            mmu->dma_source = val;
            mmu->dma_active = true;
            mmu->dma_remaining = 160u;
            mmu->memory[addr] = val;
            return;
        }
        if (addr == 0xFF00u) {
            mmu->memory[addr] = val & 0x30u;
            return;
        }
        if (addr >= 0xE000u && addr < 0xFE00u) {
            mmu->memory[addr - 0x2000u] = val;
            return;
        }
        mmu->memory[addr] = val;
        return;
    }

    if (addr < 0x2000u) {
        cart->ram_enabled = (val & 0x0Fu) == 0x0Au;
        return;
    }
    if (addr < 0x4000u) {
        if (cart->type == CARTRIDGE_MBC1) {
            cart->rom_bank = (cart->rom_bank & 0x60u) | (val & 0x1Fu);
            if ((cart->rom_bank & 0x1Fu) == 0u) cart->rom_bank |= 0x01u;
        } else if (cart->type == CARTRIDGE_MBC2) cart->rom_bank = val & 0x0Fu;
        else if (cart->type == CARTRIDGE_MBC3) cart->rom_bank = val & 0x7Fu;
        else if (cart->type == CARTRIDGE_MBC5) {
            if (addr < 0x3000u) {
                cart->rom_bank = (cart->rom_bank & 0x100u) | val;
            } else {
                cart->rom_bank = (cart->rom_bank & 0xFFu) | ((val & 0x01u) << 8);
            }
        }
        else if (cart->type == CARTRIDGE_MMM01) cart->rom_bank = val & 0x7Fu;
        if ((cart->type == CARTRIDGE_MBC1 || cart->type == CARTRIDGE_MBC2 ||
             cart->type == CARTRIDGE_MMM01) &&
            (cart->rom_bank == 0u || cart->rom_bank == 0x20u ||
             cart->rom_bank == 0x40u || cart->rom_bank == 0x60u)) {
            cart->rom_bank++;
        }
        return;
    }
    if (addr < 0x6000u) {
        if (cart->type == CARTRIDGE_MBC1) {
            cart->ram_bank = val & 0x03u;
            cart->rom_bank = (cart->rom_bank & 0x1Fu) | ((val & 0x03u) << 5);
        } else if (cart->type == CARTRIDGE_MBC3) {
            cart->ram_bank = val & 0x0Fu;
        } else if (cart->type == CARTRIDGE_MBC5) {
            cart->ram_bank = val & 0x0Fu;
        } else if (cart->type == CARTRIDGE_MBC6) {
            cart->ram_bank = val & 0x0Fu;
        }
        return;
    }
    if (addr < 0x8000u) {
        if (cart->type == CARTRIDGE_MBC3) {
            if (val == 0u) cart->rtc_latched = false;
            else if (val == 1u) cart->rtc_latched = true;
        } else if (cart->type == CARTRIDGE_MBC5) {
            cart->rom_bank = (cart->rom_bank & 0xFFu) | ((val & 0x01u) << 8);
        }
        return;
    }
    if (addr >= 0xA000u && addr < 0xC000u) {
        cartridge_ram_write(cart, addr, val);
        return;
    }
    if (addr >= 0xE000u && addr < 0xFE00u) {
        mmu->memory[addr - 0x2000u] = val;
        return;
    }
    if (addr >= 0xFEA0u && addr < 0xFF00u) {
        return;
    }
    mmu->memory[addr] = val;
}

