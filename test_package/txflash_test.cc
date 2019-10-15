#include <cstring>

#include "catch.hpp"
#include "fakeit.hpp"

#include <txflash.hh>
#include <txflash_dummy.hh>

#define CLASS_METHOD_SHOULD(class_, member_function, test) #class_ "::" #member_function " should " test, "[" #class_ "::" #member_function "]" "[" #class_ "]"

using txflash::TxFlash;
using txflash::make_txflash;
using txflash::DummyFlashBank;

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
static fakeit::Mock<DummyFlashBank<EmptyValue>> mockMemoryBank(DummyFlashBank<EmptyValue> &bank) {
    fakeit::Mock<DummyFlashBank<EmptyValue>> mock(bank);

    fakeit::Spy(Method(mock, read_chunk));
    fakeit::Spy(Method(mock, erase));
    fakeit::Spy(Method(mock, write_chunk));

    return mock;
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "honor empty value")) {
    uint8_t
            data0[20] = {},
            data1[20] = {};

    SECTION("erase when empty value does not match (empty value 0)") {
        DummyFlashBank<0> bank0(data0, 20);
        DummyFlashBank<0> bank1(data1, 20);

        auto mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

        memset(data0, 0xff, sizeof(data0));
        memset(data1, 0xff, sizeof(data1));

        make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
        fakeit::Verify(Method(mock0, erase));
        fakeit::Verify(Method(mock1, erase));
    }

    SECTION("initialize when empty matches (empty value 0)") {
        DummyFlashBank<0> bank0(data0, 20);
        DummyFlashBank<0> bank1(data1, 20);

        fakeit::Mock<DummyFlashBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

        memset(data0, 0, sizeof(data0));
        memset(data1, 0, sizeof(data1));

        make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
        fakeit::VerifyNoOtherInvocations(Method(mock0, erase));
        fakeit::VerifyNoOtherInvocations(Method(mock1, erase));
    }

    SECTION("erase when empty value does not match (empty value 0xff)") {
        DummyFlashBank<> bank0(data0, sizeof(data0));
        DummyFlashBank<> bank1(data1, sizeof(data1));

        fakeit::Mock<DummyFlashBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

        memset(data0, 0, sizeof(data0));
        memset(data1, 0, sizeof(data1));

        make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
        fakeit::Verify(Method(mock0, erase));
        fakeit::Verify(Method(mock1, erase));
    }

    SECTION("initialize when empty matches (empty value 0xff)") {
        DummyFlashBank<> bank0(data0, sizeof(data0));
        DummyFlashBank<> bank1(data1, sizeof(data1));

        fakeit::Mock<DummyFlashBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

        memset(data0, 0xff, sizeof(data0));
        memset(data1, 0xff, sizeof(data1));

        make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
        fakeit::VerifyNoOtherInvocations(Method(mock0, erase));
        fakeit::VerifyNoOtherInvocations(Method(mock1, erase));
    }
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "initialize when both banks are empty")) {
    uint8_t tmp[20],
            data0[20] = {},
            data1[20] = {};

    memset(data0, 0, sizeof(data0));
    memset(data1, 0, sizeof(data1));

    DummyFlashBank<> bank0(data0, sizeof(data0));
    DummyFlashBank<> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::Verify(Method(mock0, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "!!!!");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0001", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "wrap to next bank when full")) {
    uint8_t tmp[20],
            data0[20] = {},
            data1[20] = {};

    memset(data0, 0, sizeof(data0));
    memset(data1, 0, sizeof(data1));

    DummyFlashBank<> bank0(data0, sizeof(data0));
    DummyFlashBank<> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "0000", 5);
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    // Bank1 free = 10
    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0000");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0001", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0001");
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
    REQUIRE(std::string((const char *) tmp) == "0002");
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
    REQUIRE(std::string((const char *) tmp) == "0003****");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "handle bank#0 non-empty, bank#1 empty")) {
    uint8_t tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1, 0, sizeof(data1));

    DummyFlashBank<0> bank0(data0, sizeof(data0));
    DummyFlashBank<0> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0000");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0001", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "handle bank#0 empty, bank#1 non-empty")) {
    uint8_t tmp[20],
            data0[20] = {0},
            data1[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0};

    memset(data0, 0, sizeof(data0));
    memset(data1 + 9, 0, sizeof(data1) - 9);

    DummyFlashBank<0> bank0(data0, sizeof(data0));
    DummyFlashBank<0> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0000");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0001", 5));
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::Verify(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "handle bank#0 non-empty, bank#1 non-empty")) {
    uint8_t tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {1, 5, 0, '0', '0', '0', '1', '\0', 0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1 + 9, 0, sizeof(data1) - 9);

    DummyFlashBank<0> bank0(data0, sizeof(data0));
    DummyFlashBank<0> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0001");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0002", 5));
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::Verify(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0002");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "recover when header is invalid")) {
    uint8_t tmp[20],
            data0[20] = {0xff, 5, 0, '0', '0', '0', '0', '\0', 99},
            data1[20] = {};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1, 0, sizeof(data1));

    DummyFlashBank<0> bank0(data0, sizeof(data0));
    DummyFlashBank<0> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::Verify(Method(mock0, erase));
    fakeit::Verify(Method(mock1, erase));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "!!!!");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0002", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0002");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "recover when length is invalid")) {
    uint8_t tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {1, 9, 9, '0', '0', '0', '1', '\0', 0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1 + 9, 0, sizeof(data1) - 9);

    DummyFlashBank<> bank0(data0, sizeof(data0));
    DummyFlashBank<> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    fakeit::Verify(Method(mock0, erase));
    fakeit::Verify(Method(mock1, erase));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "!!!!");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.write("0002", 5));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    REQUIRE(tested.length() == 5);
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0002");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "support empty (0-length) default payload")) {
    uint8_t tmp[20],
            data0[20] = {0},
            data1[20] = {0};

    DummyFlashBank<0> bank0(data0, sizeof(data0));
    DummyFlashBank<0> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), nullptr, 0);
    REQUIRE(tested.length() == 0);
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash::write, "return false when payload is too big")) {
    uint8_t
            data0[20] = {0},
            data1[20] = {0};

    DummyFlashBank<0> bank0(data0, sizeof(data0));
    DummyFlashBank<0> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), nullptr, 0);
    const char long_payload[] = "this payload won't fit";
    REQUIRE(tested.write(long_payload, sizeof(long_payload)) == false);
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash::reset, "reset the flash")) {
    uint8_t tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {1, 5, 0, '0', '0', '0', '1', '\0', 0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1 + 9, 0, sizeof(data1) - 9);

    DummyFlashBank<0> bank0(data0, sizeof(data0));
    DummyFlashBank<0> bank1(data1, sizeof(data1));

    fakeit::Mock<DummyFlashBank<0>> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    auto tested = make_txflash(make_delegate(mock0.get()), make_delegate(mock1.get()), "!!!!", 5);
    // Ensure the existing data has been found
    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "0001");

    tested.reset();
    fakeit::Verify(Method(mock0, erase));
    fakeit::Verify(Method(mock1, erase));
    fakeit::Verify(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));

    tested.read(tmp);
    REQUIRE(std::string((const char *) tmp) == "!!!!");
    fakeit::VerifyNoOtherInvocations(Method(mock0, write_chunk));
    fakeit::VerifyNoOtherInvocations(Method(mock1, write_chunk));
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash::reset, "quickstart demo")) {
    uint8_t tmp[50],
            data0[50] = {0},
            data1[50] = {0};

    // Initialize the flash
    const char initial_conf[] = "default configuration";
    auto flash = txflash::make_txflash(
            DummyFlashBank<0>(data0, sizeof(data0)),
            DummyFlashBank<0>(data1, sizeof(data1)),
            initial_conf,
            sizeof(initial_conf)
    );

    // Read back the default value
    REQUIRE(flash.length() == sizeof(initial_conf));
    flash.read(tmp);
    REQUIRE(std::string((const char *) tmp) == initial_conf);

    // Now write something new and read it back
    char new_conf[] = "another configuration";
    flash.write(new_conf, sizeof(new_conf));
    REQUIRE(flash.length() == sizeof(new_conf));
    flash.read(tmp);
    REQUIRE(std::string((const char *) tmp) == new_conf);
}
