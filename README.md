# TxFlash
![License: BSD-2-Clause](https://img.shields.io/badge/License-BSD%202%20Clause-blue.svg)
[![Build Status](https://travis-ci.com/aleofreddi/txflash.svg?branch=master)](https://travis-ci.com/aleofreddi/txflash)
[![Test Coverage](https://codecov.io/gh/aleofreddi/txflash/branch/master/graph/badge.svg)](https://codecov.io/gh/aleofreddi/txflash)

## Why?

Nowadays many MCUs have an on-board flash, but not an eeprom.  While it's very common for an application to store and load various 
configuration settings (eg. networking parameters), doing so using the onboard flash can be tricky.

That's because flash can be only erased in blocks, and can sustain only a certain amount of erase cycles - so one should try
to minimize the number of erases. Also writes are not atomic, so a power-fail could happen in the middle of a write - in such
a case one should recover the last valid configuration.

TxFlash is designed exactly to address these problems!

## What?

TxFlash is a C++11 header-only library designed to provide a simple way to store and load an opaque configuration using two flash banks.

The library includes also flash bank implementations for STM32F4 and STM32F7 families, but can easily be ported to any mcu.

## Features

TxFlash' key characteristics:

- Store configurations sequentially on the same bank, retrieving the latest one on load,
- When a bank is completely used, erase to the other and switch them,
- Implement a simple transaction management to protect against mid-write reset or other partial flushes

## Quickstart

You can install TxFlash via [conan](https://conan.io), add the following dependency to your conanfile:

```
TxFlash/0.1@aleofreddi/testing
```

*NOTE:* The package has been submitted to conan central but it's not there yet - so you need to clone this repository and install it from the sources using `conan export . aleofreddi/testing && conan install TxFlash/0.1@aleofreddi/testing --build`.

Then setup the flash banks to use, in this case we are dealing with STM32F4:

```cpp
#include <txflash.hh>
#include <txflash_stm32f4.hh>

...

// Initialize the flash using flash sectors 1 & 2
const char initial_conf[] = "default configuration";
auto flash = txflash::make_txflash(
    txflash::Stm32f4FlashBank<FLASH_SECTOR_1, 0x08008000, 0x8000>(),
    txflash::Stm32f4FlashBank<FLASH_SECTOR_2, 0x08010000, 0x8000>()
    initial_conf,
    sizeof(initial_conf)
);

// Read back the default value
assert(flash.length() == sizeof(initial_conf));
flash.read(tmp);
assert(std::string(tmp) == initial_conf);

// Now write something new and read it back
char new_conf[] = "another configuration";
flash.write(new_conf, sizeof(new_conf));
assert(flash.length() == sizeof(new_conf));
flash.read(tmp);
assert(std::string(tmp) == new_conf);
```
