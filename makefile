CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wno-missing-field-initializers -O2
CPPFLAGS := -Iinclude
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
	@printf '{"directory":"%s","file":"%s","arguments":["%s","%s","%s","-c","%s","-o","%s"]}\n' \
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

