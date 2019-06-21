# TxFlash
[![License: BSD-2-Clause](https://img.shields.io/badge/License-BSD%202%20Clause-blue.svg)]
[![Build Status](https://travis-ci.com/aleofreddi/txflash.svg?branch=master)](https://travis-ci.com/aleofreddi/txflash)

## Why?

Nowadays many MCUs have an on-board flash, but not an eeprom.  While it's very common for an application to store and load various 
configuration settings (eg. networking parameters), doing so using the onboard flash can be tricky.

That's because flash can be only erased in blocks, and can sustain only a certain amount of erase cycles - so one should try
to minimize the number of erases. Also writes are not atomic, so a power-fail could happen in the middle of a write - in such
a case one should recover the last valid configuration.

TxFlash is designed exactly to address these problems!

## What?

TxFlash is a header-only library designed to provide a simple way to store and load an opaque configuration using two flash banks.

The library includes also flash bank implementations for STM32F4 and STM32F7 families, but can easily be ported to any mcu.

## Features

TxFlash' key characteristics:

- Store configurations sequentially on the same bank, retrieving the latest one on load,
- When a bank is completely used, erase to the other and switch them,
- Implement a simple transaction management to protect against mid-write reset or other partial flushes

## Quickstart

You can install TxFlash via [conan](https://conan.io):

```
# TODO
```
