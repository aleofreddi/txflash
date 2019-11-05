#ifndef TXFLASH_DUMMY_HH
#define TXFLASH_DUMMY_HH

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace txflash {

/**
 * A dummy memory buffer backed flash bank implementation. This implementation is useful for testing.
 *
 * This type is a move-only one.
 *
 * @author Andrea Leofreddi
 */
template<uint8_t EmptyValue = 0xff>
class DummyFlashBank {
public:
    static const uint8_t empty_value = EmptyValue;
    using position_t = uint16_t;

    DummyFlashBank(uint8_t *data, size_t length);

    DummyFlashBank() = delete;

    position_t length() const;

    virtual void erase();

    virtual void read_chunk(position_t position, void *destination, position_t length) const;

    virtual void write_chunk(position_t position, const void *payload, position_t length);

private:
    uint8_t const *m_flash;
    const uint16_t m_length;
};

template<uint8_t EmptyValue>
DummyFlashBank<EmptyValue>::DummyFlashBank(uint8_t *data, size_t length)
        :m_flash(data), m_length(length) {
}

template<uint8_t EmptyValue>
typename DummyFlashBank<EmptyValue>::position_t DummyFlashBank<EmptyValue>::length() const {
    return m_length;
}

template<uint8_t EmptyValue>
void DummyFlashBank<EmptyValue>::erase() {
    memset((void *) m_flash, EmptyValue, length());
};

template<uint8_t EmptyValue>
void DummyFlashBank<EmptyValue>::read_chunk(typename DummyFlashBank<EmptyValue>::position_t position, void *destination,
                                            typename DummyFlashBank<EmptyValue>::position_t length) const {
    memcpy(destination, m_flash + position, length);
};

template<uint8_t EmptyValue>
void DummyFlashBank<EmptyValue>::write_chunk(typename DummyFlashBank<EmptyValue>::position_t position, const void *payload,
                                             typename DummyFlashBank<EmptyValue>::position_t length) {
    memcpy((void *) (m_flash + position), payload, length);
};

}

#endif //TXFLASH_DUMMY_HH
