# links_start
# group: https -lssl -lcrypto
# link: -lfmt
# links_end
BUILD_DIR = build
DEBUGFLAGS = -std=c++23 -g -O0 -Wall -Wextra -Wpedantic
RELEASEFLAGS = -std=c++23 -O3 -DNDEBUG -ffast-math -flto=auto -Wall -Wextra -Wpedantic
BUILD_OUT = $(BUILD_DIR)/$(BUILD_MODE)
CXXFLAGS = $(RELEASEFLAGS)
CXX = g++
BUILD_MODE = release
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
# marker_start: ./src/test.cpp type:full
$(BUILD_OUT)/src__test.o: ./src/test.cpp
	$(CXX) $(CXXFLAGS)  -c $< -o $@

$(BUILD_OUT)/src__test: LINKS += -lfmt
$(BUILD_OUT)/src__test: $(BUILD_OUT)/src__test.o
	$(CXX) $^ -o $@ $(LINKS)

run_src__test: $(BUILD_OUT)/src__test
	$(BUILD_OUT)/src__test
# marker_end: ./src/test.cpp
