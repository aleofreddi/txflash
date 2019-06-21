import os

from conans import ConanFile, CMake, tools


class TxFlashTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    build_requires = "FakeIt/2.0.4@gasuketsu/stable", "catch2/2.2.2@bincrafters/stable"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

#    def imports(self):
#        self.copy("*.dll", dst="bin", src="bin")
#        self.copy("*.dylib*", dst="bin", src="lib")
#        self.copy('*.so*', dst='bin', src='lib')

    def test(self):
        if not tools.cross_building(self.settings):
            cmake = CMake(self)
            cmake.test()
