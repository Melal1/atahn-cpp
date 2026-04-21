# links_start
# group: https -lssl -lcrypto
# link: -lfmt
# links_end

BUILD_MODE = debug
DEBUGFLAGS = -std=c++23 -g -O0
BUILD_OUT = $(BUILD_DIR)/$(BUILD_MODE)
BUILD_DIR = build
CXX = g++
CXXFLAGS = $(DEBUGFLAGS)
RELEASEFLAGS = -std=c++23 -O3 -DNDEBUG -ffast-math -flto
$(shell mkdir -p $(BUILD_OUT))


# marker_start: ./src/main.cpp type:full
$(BUILD_OUT)/src__main.o: ./src/main.cpp
	$(CXX) $(CXXFLAGS)  -c $< -o $@

$(BUILD_OUT)/src__main: LINKS += -lfmt -lssl -lcrypto
$(BUILD_OUT)/src__main: $(BUILD_OUT)/src__main.o
	$(CXX) $^ -o $@ $(LINKS)

run_src__main: $(BUILD_OUT)/src__main
	$(BUILD_OUT)/src__main
# marker_end: ./src/main.cpp