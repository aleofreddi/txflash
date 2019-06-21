from conans import ConanFile, CMake, tools


class TxFlashConan(ConanFile):
    name = "TxFlash"
    version = "0.1"
    license = "BSD-2-Clause"
    author = "Andrea Leofreddi <a.leofreddi@quantica.io>",
    url = "https://github.com/aleofreddi/txflash"
    description = "TxFlash is a header-only library designed to provide a simple way to store and load an opaque configuration using two flash banks"
    exports_sources = ["include/*"]
    no_copy_source = True
    build_requires = "FakeIt/2.0.4@gasuketsu/stable", "catch2/2.2.2@bincrafters/stable"

    def package(self):
        self.copy("*.hh")
