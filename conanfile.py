from conans import ConanFile, CMake, tools


class TxFlashConan(ConanFile):
    name = "TxFlash"
    version = "0.1"
    license = "BSD-2-Clause"
    author = "Andrea Leofreddi <a.leofreddi@quantica.io>"
    url = "https://github.com/aleofreddi/txflash"
    description = "TxFlash is a header-only library to safely store your configurations on flash memory"
    generators = "cmake"
    build_requires = "FakeIt/2.0.4@gasuketsu/stable", "catch2/2.2.2@bincrafters/stable"
    exports_sources = ["include/*"]
    no_copy_source = True

    def package(self):
        self.copy("*.hh")
