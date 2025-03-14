# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
ifneq ($(filter tests,$(MAKECMDGOALS)),)
	FLAGS += -std=c++17
else
	FLAGS += -std=c++11
endif
CFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS += #-L$(RACK_DIR)/dep/lib -lCatch2

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

ifneq ($(filter tests,$(MAKECMDGOALS)),)
    SOURCES += $(wildcard tests/*.cpp)
    CXXFLAGS += -DPH_UNIT_TESTS -Itests
endif

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

# Catch2 unit test target
tests: all
