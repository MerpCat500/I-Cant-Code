CXX := g++
CXXFLAGS := -std=c++21 -Wall -Wextra -Wno-missing-field-initializers -O2
CPPFLAGS := -Iinclude
V4L2_CFLAGS := $(shell pkg-config --cflags libv4l2)
V4L2_LIBS := $(shell pkg-config --libs libv4l2)
SDL2_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL2_LIBS := $(shell pkg-config --libs sdl2)

CPPFLAGS += $(V4L2_CFLAGS)
CPPFLAGS += $(SDL2_CFLAGS)
LDLIBS += $(V4L2_LIBS) $(SDL2_LIBS)
MAKEFLAGS += -j$(shell nproc)

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)
COMMAND_FRAGMENTS := $(OBJECTS:.o=.json)
TARGET := build/Program.bin

.PHONY: all clean run

all: $(TARGET) compile_commands.json
	make clean
	make compile_commands.json
	make run

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@printf '{"directory":"%s","file":"%s","command":"%s %s %s -c %s -o %s"}\n' \
		"$(CURDIR)" "$<" "$(CXX)" "$(CPPFLAGS)" "$(CXXFLAGS)" "$<" "$@" > $(@:.o=.json)

compile_commands.json: $(OBJECTS)
	@{ \
		printf '[\n'; \
		first=1; \
		for fragment in $(COMMAND_FRAGMENTS); do \
			if [ $$first -eq 0 ]; then printf ',\n'; fi; \
			sed '$$ s/,[[:space:]]*$$//' "$$fragment"; \
			first=0; \
		done; \
		printf ']\n'; \
	} > $@
	@ln -sf ../$@ build/$@

clean:
	rm -rf build compile_commands.json

run: $(TARGET)
	./$(TARGET)

