#ifndef TXFLASH_HH
#define TXFLASH_HH

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <type_traits>

#ifndef TXFLASH_DEBUG
# define TXFLASH_DEBUG(...)
#endif

namespace txflash {

/**
 * Transactional flash storage. This class allows for transactional storage of arbitrary data into a two banks flash storage.
 *
 * @author Andrea Leofreddi
 */
template<typename Bank0, typename Bank1>
class TxFlash {
private:
    static_assert(Bank0::empty_value == Bank1::empty_value, "flash banks with different empty value");

    static const uint8_t empty_value = Bank0::empty_value;

    enum class Bank : bool {
        BANK0 = 0,
        BANK1 = 1
    };

    enum class Header : uint8_t {
        EMPTY = empty_value,
        RECORD = (uint8_t)((uint16_t) empty_value + 1),
        SWITCH = (uint8_t)((uint16_t) empty_value + 2)
    };

    enum class State {
        EMPTY,
        VALID,
        INVALID
    };

    using position_t = typename std::common_type<typename Bank0::position_t, typename Bank1::position_t>::type;

    const void *m_default_payload;
    const position_t m_default_payload_length;

    Bank0 m_bank0;
    Bank1 m_bank1;

    Bank m_read_bank, m_write_bank;
    position_t m_read_position, m_write_position;

    void initialize();

    void read_chunk(Bank bank, position_t position, void *destination, position_t length) const;

    void write_chunk(Bank bank, position_t position, const void *data, position_t length);

    position_t remaining(Bank bank, position_t position);

    State parse();

    State fast_forward();

public:
    /**
     * Initialize the transaction flash using the given flash banks. The default configuration will be used when flash is empty or on unrecoverable error.
     *
     * The constructed instance will take ownership of bank0 and bank1 (which will be moved into private fields).
     *
     * \param bank0 1st bank
     * \param bank1 2nd bank
     * \param default_payload Default configuration payload
     * \param length  Default configuration length
     */
    TxFlash(Bank0 &bank0, Bank1 &bank1, const void *default_payload = nullptr, position_t length = 0);

    /**
     * Initialize the transaction flash using the given flash banks. The default configuration will be used when flash is empty or on unrecoverable error.
     *
     * The constructed instance will take ownership of bank0 and bank1 (which will be moved into private fields).
     *
     * \param bank0 1st bank
     * \param bank1 2nd bank
     * \param default_payload Default configuration payload
     * \param length  Default configuration length
     */
    TxFlash(Bank0 &&bank0, Bank1 &&bank1, const void *default_payload = nullptr, position_t length = 0);

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

template<typename Bank0, typename Bank1>
TxFlash<Bank0, Bank1>::TxFlash(Bank0 &bank0, Bank1 &bank1, const void *default_payload, typename TxFlash<Bank0, Bank1>::position_t length)
        : m_bank0(bank0), m_bank1(bank1), m_default_payload(default_payload), m_default_payload_length(length) {
    initialize();
}

template<typename Bank0, typename Bank1>
TxFlash<Bank0, Bank1>::TxFlash(Bank0 &&bank0, Bank1 &&bank1, const void *default_payload, typename TxFlash<Bank0, Bank1>::position_t length)
        : m_bank0(std::move(bank0)), m_bank1(std::move(bank1)), m_default_payload(default_payload), m_default_payload_length(length) {
    initialize();
}

template<typename Bank0, typename Bank1>
void TxFlash<Bank0, Bank1>::initialize() {
    State state = parse();

    TXFLASH_DEBUG("Parsed flash, state %i, read index 0x%x@#%i, write index 0x%x@#%i\n", state, m_read_position, m_read_bank, m_write_position, m_write_bank);

    switch (state) {
        case State::INVALID:
            TXFLASH_DEBUG("Flash content is invalid\n");
            reset();
            break;

        case State::EMPTY:
            TXFLASH_DEBUG("Initializing empty flash with default payload\n");
            write(m_default_payload, m_default_payload_length);
            break;

        default:
            break;
    }
}

template<typename Bank0, typename Bank1>
typename TxFlash<Bank0, Bank1>::State TxFlash<Bank0, Bank1>::fast_forward() {
    for (Header header;;) {
        position_t length;

        if (remaining(m_read_bank, m_read_position) <
            1 /* header */ + sizeof(position_t) /* length */ + 1 /* next header */) {
            TXFLASH_DEBUG("Unexpected invalid open record at 0x%x@#%i\n", m_read_position, m_read_bank);
            return State::INVALID;
        }

        // Read length
        read_chunk(m_read_bank, m_read_position + 1 /* header */, &length, sizeof(position_t));

        if (remaining(m_read_bank, m_read_position) < 1 /* header */ + sizeof(position_t) /* length */ + length + 1 /* next header */) {
            TXFLASH_DEBUG("Unexpected invalid record length 0x%x at 0x%x@#%i\n", length, m_read_position, m_read_bank);
            return State::INVALID;
        }

        // Advance write position and read next header
        m_write_position =
                m_read_position + 1 /* header */ + sizeof(position_t) /* length */ + length /* payload */;
        read_chunk(m_read_bank, m_write_position, &header, 1);

        if (header == Header::EMPTY)
            break;

        if (header != Header::RECORD) {
            TXFLASH_DEBUG("Unexpected header 0x%x at 0x%x@#%i\n", header, m_write_position, m_read_bank);
            return State::INVALID;
        }

        m_read_position = m_write_position;
    }

    return State::VALID;
}

template<typename Bank0, typename Bank1>
typename TxFlash<Bank0, Bank1>::State TxFlash<Bank0, Bank1>::parse() {
    Header headerBank0, headerBank1;

    // Reset pointers
    m_read_bank = m_write_bank = Bank::BANK0;
    m_read_position = m_write_position = 0;

    // Read BANK0 header
    read_chunk(Bank::BANK0, 0, &headerBank0, 1);
    read_chunk(Bank::BANK1, 0, &headerBank1, 1);

    // If bank0 seems empty, verify bank1
    TXFLASH_DEBUG("Bank0 %s, bank1 %s\n",
                  headerBank0 == Header::EMPTY ? "empty" : "non-empty",
                  headerBank1 == Header::EMPTY ? "empty" : "non-empty");

    if (headerBank0 == Header::EMPTY && headerBank1 == Header::EMPTY) {
        TXFLASH_DEBUG("Empty flash, initializing with default payload\n");
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
        TXFLASH_DEBUG("Corrupted, unrecoverable payload. Initializing with default payload\n");
        return State::INVALID;
    }
}

template<typename Bank0, typename Bank1>
typename TxFlash<Bank0, Bank1>::position_t TxFlash<Bank0, Bank1>::length() const {
    position_t length;
    read_chunk(m_read_bank, m_read_position + 1, &length, sizeof(position_t));
    return length;
}

template<typename Bank0, typename Bank1>
typename TxFlash<Bank0, Bank1>::position_t
TxFlash<Bank0, Bank1>::remaining(Bank bank, position_t position) {
    return bank == Bank::BANK0 ? m_bank0.length() - position : m_bank1.length() - position;
}

template<typename Bank0, typename Bank1>
void TxFlash<Bank0, Bank1>::read_chunk(Bank bank, position_t position, void *destination,
                                       position_t length) const {
    return bank == Bank::BANK0 ? m_bank0.read_chunk(position, destination, length)
                               : m_bank1.read_chunk(position, destination, length);
}

template<typename Bank0, typename Bank1>
void TxFlash<Bank0, Bank1>::write_chunk(Bank bank, position_t position, const void *destination,
                                        position_t length) {
    return bank == Bank::BANK0 ? m_bank0.write_chunk(position, destination, length)
                               : m_bank1.write_chunk(position, destination, length);
}

template<typename Bank0, typename Bank1>
void TxFlash<Bank0, Bank1>::read(void *destination) const {
    position_t length = this->length();
    return read_chunk(m_read_bank, m_read_position + 1 /* header */ + sizeof(position_t) /* length */, destination, length);
}

template<typename Bank0, typename Bank1>
bool TxFlash<Bank0, Bank1>::write(const void *payload, position_t length) {
    if (std::min(remaining(Bank::BANK0, 0), remaining(Bank::BANK1, 0)) <
        1 /* header */ + sizeof(position_t) /* length */ + length /* payload */ + 1 /* next header */) {
        TXFLASH_DEBUG("Payload exceeds bank size\n");
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

template<typename Bank0, typename Bank1>
void TxFlash<Bank0, Bank1>::reset() {
    TXFLASH_DEBUG("Resetting flash to default value\n");

    m_bank0.erase();
    m_bank1.erase();

    m_read_bank = m_write_bank = Bank::BANK0;
    m_read_position = m_write_position = 0;

    write(m_default_payload, m_default_payload_length);
}

/**
 * Factory function to instance a TxFlash.
 *
 * \tparam Bank0 Bank0 type
 * \tparam Bank1 Bank1 type
 * \param bank0 Bank0 implementation
 * \param bank1 Bank1 implementation
 * \param default_payload Default payload
 * \param default_length Default payload length
 * \return
 */
template<typename Bank0, typename Bank1>
TxFlash<
        typename std::remove_reference<Bank0>::type,
        typename std::remove_reference<Bank1>::type
> make_txflash(Bank0 &&bank0, Bank1 &&bank1, const void *default_payload,
               typename std::common_type<
                       typename std::remove_reference<Bank0>::type::position_t,
                       typename std::remove_reference<Bank1>::type::position_t
               >::type default_length
) {
    return TxFlash<
            typename std::remove_reference<Bank0>::type,
            typename std::remove_reference<Bank1>::type
    >(
            std::forward<Bank0>(bank0),
            std::forward<Bank1>(bank1),
            default_payload,
            default_length
    );
}

}

#endif //TXFLASH_HH
