from conans import ConanFile, CMake, tools


class TxflashConan(ConanFile):
    name = "TxFlash"
    version = "0.1"
    license = "BSD-2"
    author = "Andrea Leofreddi <a.leofreddi@quantica.io>",
    url = "https://github.com/aleofreddi/txflash"
    description = "TxFlash is a header-only library designed to provide a simple way to store and load an opaque configuration using two flash banks"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    exports_sources = ["include/*", "test_package/*", "CMakeLists.txt"]
    build_requires = "FakeIt/2.0.4@gasuketsu/stable", "catch2/2.2.2@bincrafters/stable"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy("*.hh", dst="include", src="include")

    def package_info(self):
        self.cpp_info.libs = []
