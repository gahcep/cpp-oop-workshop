STAGES := 01 02 03 04 05 06 07 08 09 10
BUILD ?= build
CXXFLAGS ?= -O0 -g -Wall -Wextra -Wpedantic
PROGRAMS := $(addprefix $(BUILD)/stage,$(STAGES))

.PHONY: all check clean $(addprefix run-,$(STAGES))
all: $(PROGRAMS)

$(BUILD):
	mkdir -p "$@"

$(BUILD)/stage10: stage10.cpp | $(BUILD)
	"$(CXX)" $(CPPFLAGS) $(CXXFLAGS) -std=c++23 "$<" $(LDFLAGS) $(LDLIBS) -o "$@"

$(BUILD)/stage%: stage%.cpp | $(BUILD)
	"$(CXX)" $(CPPFLAGS) $(CXXFLAGS) -std=c++20 "$<" $(LDFLAGS) $(LDLIBS) -o "$@"

$(addprefix run-,$(STAGES)): run-%: $(BUILD)/stage%
	"./$<"

check: all
	@set -e; for stage in $(PROGRAMS); do printf '\n== %s ==\n' "$$stage"; "./$$stage"; done

clean:
	$(RM) $(PROGRAMS)
