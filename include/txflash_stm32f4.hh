#ifndef TXFLASH_STM32F4_HH
#define TXFLASH_STM32F4_HH

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_flash_ex.h>

namespace txflash {

/**
 * Flash bank implementation backed by ST's HAL for the STM32F4 family.
 *
 * This type is a move-only one.
 *
 * \tparam Sector Flash sector number (eg. FLASH_SECTOR_1)
 * \tparam Address Memory address (eg. 0x08008000)
 * \tparam Length Length (eg. 0x8000)
 *
 * @author Andrea Leofreddi
 */
template<uint8_t Sector, uint32_t Address, uint32_t Length>
class Stm32f4FlashBank {
public:
    static const uint8_t empty_value = 0xff;
    using position_t = size_t;

    Stm32f4FlashBank() = default;
    Stm32f4FlashBank(Stm32f4FlashBank &) = delete;
    Stm32f4FlashBank(Stm32f4FlashBank &&) = default;

    size_t length() const;
    void erase();
    void read_chunk(size_t position, void *destination, size_t length) const;
    void write_chunk(size_t position, const void *payload, size_t length);
};

template<uint8_t Sector, uint32_t Address, uint32_t Length>
size_t Stm32f4FlashBank<Sector, Address, Length>::length() const {
    return Length;
}

template<uint8_t Sector, uint32_t Address, uint32_t Length>
void Stm32f4FlashBank<Sector, Address, Length>::erase() {
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(Sector, VOLTAGE_RANGE_3);
    HAL_FLASH_Lock();
}

template<uint8_t Sector, uint32_t Address, uint32_t Length>
void Stm32f4FlashBank<Sector, Address, Length>::read_chunk(size_t position, void *destination, size_t length) const {
    memcpy(destination, (char *) Address + position, length);
}

template<uint8_t Sector, uint32_t Address, uint32_t Length>
void Stm32f4FlashBank<Sector, Address, Length>::write_chunk(size_t position, const void *source, size_t length) {
    assert(position + length <= Length);
    char *current = (char *) Address + position, *end = current + length, *read = (char *) source;
    HAL_FLASH_Unlock();

    for(; (uint32_t) current % 4 && current < end; current++, read++)
        if(HAL_FLASH_Program(TYPEPROGRAM_BYTE, (uint32_t) current, (uint8_t) *read) != HAL_OK)
            Error_Handler();

    for(; current + 4 < end; current += 4, read += 4)
        if(HAL_FLASH_Program(TYPEPROGRAM_WORD, (uint32_t) current, *((uint32_t *) read)) != HAL_OK)
            Error_Handler();

    for(; current < end; current++, read++)
        if(HAL_FLASH_Program(TYPEPROGRAM_BYTE, (uint32_t) current, (uint8_t) *read) != HAL_OK)
            Error_Handler();

    HAL_FLASH_Lock();
}

}

#endif //TXFLASH_STM32F4_HH
