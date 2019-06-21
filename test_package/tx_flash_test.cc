#include <cstring>

#include "catch.hpp"
#include "fakeit.hpp"

#include <tx_flash.hh>

#define CLASS_METHOD_SHOULD(class_, member_function, test) #class_ "::" #member_function " should " test, "[" #class_ "::" #member_function "]" "[" #class_ "]"

using txflash::TxFlash;

/**
 * A memory-based flash bank emulator.
 *
 * @author Andrea Leofreddi
 */
class MemoryBank {
private:
    uint8_t *m_begin, *m_end;

    const char *m_id;

public:
    using position_t = uint16_t;

    MemoryBank(const char *id, void *begin, void *end) : m_id(id), m_begin(static_cast<uint8_t *>(begin)), m_end(static_cast<uint8_t *>(end)) {
    }

    virtual position_t length() const {
        return static_cast<position_t>(m_end - m_begin);
    }

    virtual void erase() {
        memset(m_begin, 0, m_end - m_begin);
    }

    virtual void read_chunk(position_t position, void *destination, position_t length) const {
        memcpy(destination, m_begin + position, length);
    }

    virtual void write_chunk(position_t position, const void *payload, position_t length) {
        memcpy(m_begin + position, payload, length);
    }
};

/**
 * Initializes a spy on the given memory bank.
 *
 * @param bank Bank to spy
 * @return Banked spy
 */
static fakeit::Mock<MemoryBank> mockMemoryBank(MemoryBank &bank) {
    fakeit::Mock<MemoryBank> mock(bank);

    fakeit::Spy(Method(mock, read_chunk));
    fakeit::Spy(Method(mock, erase));
    fakeit::Spy(Method(mock, write_chunk));

    return mock;
}

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash, "honor empty value")) {
    char
            data0[20] = {},
            data1[20] = {};

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    SECTION("erase when empty value does not match (empty value 0)") {
        memset(data0, 0xff, sizeof(data0));
        memset(data1, 0xff, sizeof(data1));

        TxFlash<MemoryBank, MemoryBank, 0>(
                mock0.get(),
                mock1.get(),
                "!!!!", 5
        );
        fakeit::Verify(Method(mock0, erase));
        fakeit::Verify(Method(mock1, erase));
    }

    SECTION("initialize when empty matches (empty value 0)") {
        memset(data0, 0, sizeof(data0));
        memset(data1, 0, sizeof(data1));

        TxFlash<MemoryBank, MemoryBank, 0>(
                mock0.get(),
                mock1.get(),
                "!!!!", 5
        );
        fakeit::VerifyNoOtherInvocations(Method(mock0, erase));
        fakeit::VerifyNoOtherInvocations(Method(mock1, erase));
    }

    SECTION("erase when empty value does not match (empty value 0xff)") {
        memset(data0, 0, sizeof(data0));
        memset(data1, 0, sizeof(data1));

        TxFlash<MemoryBank, MemoryBank, 0xff>(
                mock0.get(),
                mock1.get(),
                "!!!!", 5
        );
        fakeit::Verify(Method(mock0, erase));
        fakeit::Verify(Method(mock1, erase));
    }

    SECTION("initialize when empty matches (empty value 0xff)") {
        memset(data0, 0xff, sizeof(data0));
        memset(data1, 0xff, sizeof(data1));

        TxFlash<MemoryBank, MemoryBank, 0xff>(
                mock0.get(),
                mock1.get(),
                "!!!!", 5
        );
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

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    TxFlash<MemoryBank, MemoryBank> tested(
            mock0.get(),
            mock1.get(),
            "!!!!", 5
    );
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

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    TxFlash<MemoryBank, MemoryBank, 0> tested(
            mock0.get(),
            mock1.get(),
            "0000", 5
    );
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

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    TxFlash<MemoryBank, MemoryBank, 0> tested(
            mock0.get(),
            mock1.get(),
            "!!!!", 5
    );
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

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    TxFlash<MemoryBank, MemoryBank, 0> tested(
            mock0.get(),
            mock1.get(),
            "!!!!",
            5
    );
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

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    TxFlash<MemoryBank, MemoryBank, 0> tested(
            mock0.get(),
            mock1.get(),
            "!!!!", 5
    );
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

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    TxFlash<MemoryBank, MemoryBank> tested(
            mock0.get(),
            mock1.get(),
            "!!!!", 5
    );
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

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    TxFlash<MemoryBank, MemoryBank> tested(
            mock0.get(),
            mock1.get(),
            "!!!!", 5
    );
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

TEST_CASE(CLASS_METHOD_SHOULD(TxFlash, TxFlash::reset, "reset the flash")) {
    char tmp[20],
            data0[20] = {1, 5, 0, '0', '0', '0', '0', '\0', 0},
            data1[20] = {1, 5, 0, '0', '0', '0', '1', '\0', 0};

    memset(data0 + 9, 0, sizeof(data0) - 9);
    memset(data1 + 9, 0, sizeof(data1) - 9);

    MemoryBank bank0("#0", data0, data0 + sizeof(data0));
    MemoryBank bank1("#1", data1, data1 + sizeof(data1));

    fakeit::Mock<MemoryBank> mock0(mockMemoryBank(bank0)), mock1(mockMemoryBank(bank1));

    TxFlash<MemoryBank, MemoryBank, 0> tested(
            mock0.get(),
            mock1.get(),
            "!!!!", 5
    );

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
