#include <cstring>

#include "catch.hpp"
#include "fakeit.hpp"

#include <tx_flash.hh>

#define CLASS_METHOD_SHOULD(class_, member_function, test) #class_ "::" #member_function " should " test, "[" #class_ "::" #member_function "]" "[" #class_ "]"

using txflash::TxFlash;
using txflash::make_txflash;

/**
 * A memory-based flash bank emulator.
 *
 * @author Andrea Leofreddi
 */
template<uint8_t EmptyValue = 0xff>
class MemoryBank {
private:
    uint8_t *m_begin, *m_end;
    const char *m_id;

public:
    static const uint8_t empty_value = EmptyValue;
    using position_t = uint16_t;

    MemoryBank(const char *id, void *begin, void *end) : m_id(id), m_begin(static_cast<uint8_t *>(begin)), m_end(static_cast<uint8_t *>(end)) {
    }

    virtual position_t length() const {
        return static_cast<position_t>(m_end - m_begin);
    }

    virtual void erase() {
        memset(m_begin, EmptyValue, m_end - m_begin);
    }

    virtual void read_chunk(position_t position, void *destination, position_t length) const {
        memcpy(destination, m_begin + position, length);
    }

    virtual void write_chunk(position_t position, const void *payload, position_t length) {
        memcpy(m_begin + position, payload, length);
    }
};

/**
 * As fakeit mocks won't play well when copied or moved, we'll wrap these into a delegating bank.
 *
 * @tparam T
 */
template<class T>
class DelegateBank {
private:
    T *m_delegate;

public:
    using position_t = typename T::position_t;
    const static uint8_t empty_value = T::empty_value;

    DelegateBank(T *delegate) : m_delegate(delegate) {
    }

    position_t length() const {
        return m_delegate->length();
    }

    virtual void erase() {
        return m_delegate->erase();
    }

    virtual void read_chunk(position_t position, void *destination, position_t length) const {
        return m_delegate->read_chunk(position, destination, length);
    }

    virtual void write_chunk(position_t position, const void *payload, position_t length) {
        return m_delegate->write_chunk(position, payload, length);
    }
};

template<class T>
DelegateBank<T> make_delegate(T &t) {
    return DelegateBank<T>(&t);
}

/**
 * Initializes a spy on the given memory bank.
 *
 * @param bank Bank to spy
 * @return Banked spy
 */
template<uint8_t EmptyValue>
static fakeit::Mock<MemoryBank<EmptyValue>> mockMemoryBank(MemoryBank<EmptyValue> &bank) {
    fakeit::Mock<MemoryBank<EmptyValue>> mock(bank);

    fakeit::Spy(Method(mock, read_chunk));
    fakeit::Spy(Method(mock, erase));
    fakeit::Spy(Method(mock, write_chunk));

    return mock;
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "honor empty value")) {
    char
            data0[20] = {},
            data1[20] = {};

    SECTION("erase when empty value does not match (empty value 0)") {
        MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
        MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

        auto mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

        memset(data0, 0xff, sizeof(data0));
        memset(data1, 0xff, sizeof(data1));

        make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
        fakeit::Verify(Method(mock0, erase));
        fakeit::Verify(Method(mock1, erase));
    }

    SECTION("initialize when empty matches (empty value 0)") {
        MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
        MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

        fakeit::Mock<MemoryBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

        memset(data0, 0, sizeof(data0));
        memset(data1, 0, sizeof(data1));

        make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
        fakeit::VerifyNoOtherInvocations(Method(mock0, erase));
        fakeit::VerifyNoOtherInvocations(Method(mock1, erase));
    }

    SECTION("erase when empty value does not match (empty value 0xff)") {
        MemoryBank<> bank0("#0", data0, data0 + sizeof(data0));
        MemoryBank<> bank1("#1", data1, data1 + sizeof(data1));

        fakeit::Mock<MemoryBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

        memset(data0, 0, sizeof(data0));
        memset(data1, 0, sizeof(data1));

        make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
        fakeit::Verify(Method(mock0, erase));
        fakeit::Verify(Method(mock1, erase));
    }

    SECTION("initialize when empty matches (empty value 0xff)") {
        MemoryBank<> bank0("#0", data0, data0 + sizeof(data0));
        MemoryBank<> bank1("#1", data1, data1 + sizeof(data1));

        fakeit::Mock<MemoryBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

        memset(data0, 0xff, sizeof(data0));
        memset(data1, 0xff, sizeof(data1));

        make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
        fakeit::VerifyNoOtherInvocations(Method(mock0, erase));
        fakeit::VerifyNoOtherInvocations(Method(mock1, erase));
    }
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "initialize when both banks are empty")) {
    char tmp[20],
            data0[20] = {},
            data1[20] = {};

    memset(data0, 0, sizeof(data0));
    memset(data1, 0, sizeof(data1));

    MemoryBank<> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::Verify(Method(mock0, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string(tmp) == "!!!!");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0001", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "wrap to next bank when full")) {
    char tmp[20],
            data0[20] = {},
            data1[20] = {};

    memset(data0, 0, sizeof(data0));
    memset(data1, 0, sizeof(data1));

    MemoryBank<> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "0000", 5);
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    // Bank1 free = 10
    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0000");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0001", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    // Ensure the next write goes to bank1
    REQUIRE(tested.write("0002", 5));
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::Verify(
            Method(mock1, erase) +
            Method(mock1, write_chunk) * 3
    );

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0002");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    // Ensure the next write goes back to bank0 and after bank1 gets erase
    REQUIRE(tested.write("0003****", 9));
    fakeit::Verify(
            Method(mock0, erase)
            + Method(mock0, write_chunk) * 3
            + Method(mock1, erase)
    );
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 9);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0003****");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "handle bank#0 non-empty, bank#1 empty")) {
    char tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1, 0, sizeof(data1));

    MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0000");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0001", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "handle bank#0 empty, bank#1 non-empty")) {
    char tmp[20],
            data0[20] = {0},
            data1[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0};

    memset(data0, 0, sizeof(data0));
    memset(data1 + 9, 0, sizeof(data1) - 9);

    MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0000");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0001", 5));
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::Verify(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "handle bank#0 non-empty, bank#1 non-empty")) {
    char tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {1, 5, 0, '0', '0', '0', '1', '\0', 0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1 + 9, 0, sizeof(data1) - 9);

    MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0002", 5));
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::Verify(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0002");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "recover when header is invalid")) {
    char tmp[20],
            data0[20] = {(char) 0xff, 5, 0, '0', '0', '0', '0', '\0', 99},
            data1[20] = {};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1, 0, sizeof(data1));

    MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::Verify(Method(mock0, erase));
    fakeit::Verify(Method(mock1, erase));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string(tmp) == "!!!!");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0002", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0002");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "recover when length is invalid")) {
    char tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {1, 9, 9, '0', '0', '0', '1', '\0', 0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1 + 9, 0, sizeof(data1) - 9);

    MemoryBank<> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::Verify(Method(mock0, erase));
    fakeit::Verify(Method(mock1, erase));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string(tmp) == "!!!!");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0002", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0002");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "support empty (0-length) default payload")) {
    char tmp[20],
            data0[20] = {0},
            data1[20] = {0};

    MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), nullptr, 0);
    REQUIRE(tested.length() == 0);
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash::write, "return false when payload is too big")) {
    char tmp[20],
            data0[20] = {0},
            data1[20] = {0};

    MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), nullptr, 0);
    const char long_payload[] = "this payload won't fit";
    REQUIRE(tested.write(long_payload, sizeof(long_payload)) == false);
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash::reset, "reset the flash")) {
    char tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {1, 5, 0, '0', '0', '0', '1', '\0', 0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1 + 9, 0, sizeof(data1) - 9);

    MemoryBank<0> bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank<0> bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    // Ensure the existing data has been found
    tested.read(tmp);
    REQUIRE(std::string(tmp) == "0001");

    tested.reset();
    fakeit::Verify(Method(mock0, erase));
    fakeit::Verify(Method(mock1, erase));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string(tmp) == "!!!!");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash::reset, "quickstart demo")) {
    char tmp[50],
            data0[50] = {0},
            data1[50] = {0};

    // Initialize the flash
    const char initial_conf[] = "default configuration";
    auto flash = txflash::make_txflash(
        MemoryBank<0>("#0", data0, data0 + sizeof(data0)),
        MemoryBank<0>("#1", data1, data1 + sizeof(data1)),
        initial_conf,
        sizeof(initial_conf)
    );

    // Read back the default value
    REQUIRE(flash.length() == sizeof(initial_conf));
    flash.read(tmp);
    REQUIRE(std::string(tmp) == initial_conf);

    // Now write something new and read it back
    char new_conf[] = "another configuration";
    flash.write(new_conf, sizeof(new_conf));
    REQUIRE(flash.length() == sizeof(new_conf));
    flash.read(tmp);
    REQUIRE(std::string(tmp) == new_conf);
}
