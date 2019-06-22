#ifndef TX_FLASH_HH
#define TX_FLASH_HH

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <type_traits>

#ifndef TX_FLASH_DEBUG
# define TX_FLASH_DEBUG(...)
#endif

namespace txflash {

/**
 * Transactional flash storage. This class allows for transactional storage of arbitrary data into a two banks flash storage.
 *
 * @author Andrea Leofreddi
 */
template<typename Bank0, typename Bank1, uint8_t EmptyValue = 0xff>
class TxFlash {
private:
    enum class Bank : bool {
        BANK0 = 0,
        BANK1 = 1
    };

    enum class Header : uint8_t {
        EMPTY = EmptyValue,
        RECORD = (uint8_t)((uint16_t) EmptyValue + 1),
        SWITCH = (uint8_t)((uint16_t) EmptyValue + 2)
    };

    enum class State {
        EMPTY,
        VALID,
        INVALID
    };

    using position_t = typename std::common_type<typename Bank0::position_t, typename Bank1::position_t>::type;

    const void *m_default_payload;
    const position_t m_default_payload_length;

    Bank0 &m_bank0;
    Bank1 &m_bank1;

    Bank m_read_bank, m_write_bank;
    position_t m_read_position, m_write_position;

    void read_chunk(Bank bank, position_t position, void *destination, position_t length) const;

    void write_chunk(Bank bank, position_t position, const void *data, position_t length);

    position_t remaining(Bank bank, position_t position);

    State parse();

    State fast_forward();

public:
    /**
     * Initialize the transaction flash using the given flash banks. The default configuration will be used when flash is empty or an unrecoverable error
     * happenes.
     *
     * \param bank0 1st bank
     * \param bank1 2nd bank
     * \param default_payload Default configuration payload
     * \param length  Default configuration length
     */
    TxFlash(Bank0 &bank0, Bank1 &bank1, const void *default_payload, position_t length);

    /**
     * Retrieve the current configuration length.
     *
     * \return Configuration length
     */
    position_t length() const;

    /**
     * Load configuration and copies it into the destination buffer, which must be able to contain at least length() bytes.
     *
     * \param destination Destination buffer where to store the configuration
     */
    void read(void *destination) const;

    /**
     * Store a new configuration.
     *
     * \param payload The configuration to store
     * \param length Length of the configuration to store
     * \return True if the operations succeed, else return false (eg. when the payload doesn't fit the flash due to excessive length)
     */
    bool write(const void *payload, position_t length);

    /**
     * Reset the configuration to the default one.
     */
    void reset();
};

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
TxFlash<Bank0, Bank1, EmptyValue>::TxFlash(Bank0 &bank0, Bank1 &bank1, const void *default_payload, typename TxFlash<Bank0, Bank1, EmptyValue>::position_t length)
        : m_bank0(bank0), m_bank1(bank1), m_default_payload(default_payload), m_default_payload_length(length) {
    State state = parse();

    TX_FLASH_DEBUG("Parsed flash, state %i, read index 0x%x@#%i, write index 0x%x@#%i\n", state, m_read_position, m_read_bank, m_write_position, m_write_bank);

    switch (state) {
        case State::INVALID:
            TX_FLASH_DEBUG("Flash content is invalid\n");
            reset();
            break;

        case State::EMPTY:
            TX_FLASH_DEBUG("Initializing empty flash with default payload\n");
            write(default_payload, length);
            break;

        default:
            break;
    }
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
typename TxFlash<Bank0, Bank1, EmptyValue>::State TxFlash<Bank0, Bank1, EmptyValue>::fast_forward() {
    for (Header header;;) {
        position_t length;

        if (remaining(m_read_bank, m_read_position) <
            1 /* header */ + sizeof(position_t) /* length */ + 1 /* next header */) {
            TX_FLASH_DEBUG("Unexpected invalid open record at 0x%x@#%i\n", m_read_position, m_read_bank);
            return State::INVALID;
        }

        // Read length
        read_chunk(m_read_bank, m_read_position + 1 /* header */, &length, sizeof(position_t));

        if (remaining(m_read_bank, m_read_position) < 1 /* header */ + sizeof(position_t) /* length */ + length + 1 /* next header */) {
            TX_FLASH_DEBUG("Unexpected invalid record length 0x%x at 0x%x@#%i\n", length, m_read_position, m_read_bank);
            return State::INVALID;
        }

        // Advance write position and read next header
        m_write_position =
                m_read_position + 1 /* header */ + sizeof(position_t) /* length */ + length /* payload */;
        read_chunk(m_read_bank, m_write_position, &header, 1);

        if (header == Header::EMPTY)
            break;

        if (header != Header::RECORD) {
            TX_FLASH_DEBUG("Unexpected header 0x%x at 0x%x@#%i\n", header, m_write_position, m_read_bank);
            return State::INVALID;
        }

        m_read_position = m_write_position;
    }

    return State::VALID;
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
typename TxFlash<Bank0, Bank1, EmptyValue>::State TxFlash<Bank0, Bank1, EmptyValue>::parse() {
    Header headerBank0, headerBank1;

    // Reset pointers
    m_read_bank = m_write_bank = Bank::BANK0;
    m_read_position = m_write_position = 0;

    // Read BANK0 header
    read_chunk(Bank::BANK0, 0, &headerBank0, 1);
    read_chunk(Bank::BANK1, 0, &headerBank1, 1);

    // If bank0 seems empty, verify bank1
    TX_FLASH_DEBUG("Bank0 %s, bank1 %s\n",
                   headerBank0 == Header::EMPTY ? "empty" : "non-empty",
                   headerBank1 == Header::EMPTY ? "empty" : "non-empty");

    if (headerBank0 == Header::EMPTY && headerBank1 == Header::EMPTY) {
        TX_FLASH_DEBUG("Empty flash, initializing with default payload\n");
        return State::EMPTY;
    } else if (headerBank0 == Header::EMPTY && headerBank1 == Header::RECORD) {
        m_read_bank = m_write_bank = Bank::BANK1;
        return fast_forward();
    } else if (headerBank0 == Header::RECORD && headerBank1 == Header::EMPTY) {
        return fast_forward();
    } else if (headerBank0 == Header::RECORD && headerBank1 == Header::RECORD) {
        m_read_bank = m_write_bank = Bank::BANK1;
        return fast_forward();
    } else {
        TX_FLASH_DEBUG("Corrupted, unrecoverable payload. Initializing with default payload\n");
        return State::INVALID;
    }
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
typename TxFlash<Bank0, Bank1, EmptyValue>::position_t TxFlash<Bank0, Bank1, EmptyValue>::length() const {
    position_t length;
    read_chunk(m_read_bank, m_read_position + 1, &length, sizeof(position_t));
    return length;
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
typename TxFlash<Bank0, Bank1, EmptyValue>::position_t
TxFlash<Bank0, Bank1, EmptyValue>::remaining(Bank bank, position_t position) {
    return bank == Bank::BANK0 ? m_bank0.length() - position : m_bank1.length() - position;
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
void TxFlash<Bank0, Bank1, EmptyValue>::read_chunk(Bank bank, position_t position, void *destination,
                                                   position_t length) const {
    return bank == Bank::BANK0 ? m_bank0.read_chunk(position, destination, length)
                               : m_bank1.read_chunk(position, destination, length);
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
void TxFlash<Bank0, Bank1, EmptyValue>::write_chunk(Bank bank, position_t position, const void *destination,
                                                    position_t length) {
    return bank == Bank::BANK0 ? m_bank0.write_chunk(position, destination, length)
                               : m_bank1.write_chunk(position, destination, length);
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
void TxFlash<Bank0, Bank1, EmptyValue>::read(void *destination) const {
    position_t length = this->length();
    return read_chunk(m_read_bank, m_read_position + 1 /* header */ + sizeof(position_t) /* length */, destination, length);
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
bool TxFlash<Bank0, Bank1, EmptyValue>::write(const void *payload, position_t length) {
    if (std::min(remaining(Bank::BANK0, 0), remaining(Bank::BANK1, 0)) <
        1 /* header */ + sizeof(position_t) /* length */ + length /* payload */ + 1 /* next header */) {
        TX_FLASH_DEBUG("Payload exceeds bank size\n");
        return false;
    }

    if (remaining(m_write_bank, m_write_position) >= 1 /* header */ + sizeof(position_t) /* length */ + length /* payload */ + 1 /* next header */) {
        // Write length
        write_chunk(m_write_bank, m_write_position + 1 /* header */, &length, sizeof(position_t));

        // Write payload
        write_chunk(m_write_bank, m_write_position + 1 /* header */ + sizeof(position_t) /* length */, payload, length);

        // Write header
        Header header = Header::RECORD;
        write_chunk(m_write_bank, m_write_position, &header, 1);

        m_read_bank = m_write_bank;
        m_read_position = m_write_position;

        m_write_position += 1 /* header */ + sizeof(position_t) /* length */ + length /* payload */;

        return true;
    } else {
        Bank target_bank = m_write_bank == Bank::BANK0 ? Bank::BANK1 : Bank::BANK0;
        m_write_position = 0;

        bool result;

        switch (target_bank) {
            case Bank::BANK1:
                m_bank1.erase();
                m_write_bank = Bank::BANK1;
                result = write(payload, length);
                break;

            case Bank::BANK0:
                m_bank0.erase();
                m_write_bank = Bank::BANK0;
                result = write(payload, length);
                if (result)
                    m_bank1.erase();
                break;
        }

        return result;
    }
}

template<typename Bank0, typename Bank1, uint8_t EmptyValue>
void TxFlash<Bank0, Bank1, EmptyValue>::reset() {
    TX_FLASH_DEBUG("Resetting flash to default value\n");

    m_bank0.erase();
    m_bank1.erase();

    m_read_bank = m_write_bank = Bank::BANK0;
    m_read_position = m_write_position = 0;

    write(m_default_payload, m_default_payload_length);
}

}

#endif //TX_FLASH_HH
