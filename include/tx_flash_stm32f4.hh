#ifndef TX_FLASH_STM32F4_HH
#define TX_FLASH_STM32F4_HH

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
 * Use as follows:
 *
 * #@code
 * using FlashBank0 = Stm32f4FlashBank<FLASH_SECTOR_1, 0x08008000, 0x8000>;
 * using FlashBank1 = Stm32f4FlashBank<FLASH_SECTOR_2, 0x08010000, 0x8000>;
 *
 * using Flash = TxFlash<FlashBank0, FlashBank1>;
 * #@endcode
 *
 * @author Andrea Leofreddi
 */
template<uint8_t Sector, uint32_t Address, uint32_t Length>
class Stm32f4FlashBank {
public:
    using position_t = size_t;

    Stm32f4FlashBank();

    size_t length() const;
    void erase();
    void read_chunk(size_t position, void *destination, size_t length) const;
    void write_chunk(size_t position, const void *payload, size_t length);
};

template<uint8_t Sector, uint32_t Address, uint32_t Length>
Stm32f4FlashBank<Sector, Address, Length>::Stm32f4FlashBank() {
}

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

#endif //TX_FLASH_STM32F4_HH
