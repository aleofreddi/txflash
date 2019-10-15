#ifndef TXFLASH_DUMMY_HH
#define TXFLASH_DUMMY_HH

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_flash_ex.h>

namespace txflash {

/**
 * A dummy memory buffer backed flash bank implementation. This implementation is useful for testing.
 *
 * This type is a move-only one.
 *
 * @author Andrea Leofreddi
 */
template<int Id, size_t Length>
class MemoryBank {
public:
    using position_t = uint16_t;

    MemoryBank() = default;

    position_t length() const;

    void erase();

    void read_chunk(position_t position, void *destination, position_t length) const;

    void write_chunk(position_t position, const void *payload, position_t length);

private:
    static inline uint8_t m_flash[Length];
};

template<int Id, size_t Length>
typename MemoryBank<Id, Length>::position_t MemoryBank<Id, Length>::length() const {
    return Length;
}

template<int Id, size_t Length>
void MemoryBank<Id, Length>::erase() {
    memset(m_flash, 0, sizeof(m_flash));
};

template<int Id, size_t Length>
void MemoryBank<Id, Length>::read_chunk(typename MemoryBank<Id, Length>::position_t position, void *destination,
                                        typename MemoryBank<Id, Length>::position_t length) const {
    memcpy(destination, m_flash + position, length);
};

template<int Id, size_t Length>
void MemoryBank<Id, Length>::write_chunk(typename MemoryBank<Id, Length>::position_t position, const void *payload,
                                         typename MemoryBank<Id, Length>::position_t length) {
    memcpy(m_flash + position, payload, length);
};

}

#endif //TXFLASH_DUMMY_HH
