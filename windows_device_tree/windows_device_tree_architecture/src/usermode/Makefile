# Makefile for Windows Device Enumerator (cross-compile or WSL)
# For native Windows: use cl.exe directly or MSBuild

TARGET = enum_devices.exe
CXX = x86_64-w64-mingw32-g++
CXXFLAGS = -Wall -Wextra -Werror -O2 -std=c++17
LDFLAGS = -lsetupapi -lcfgmgr32 -static-libgcc -static-libstdc++

.PHONY: all clean

all: $(TARGET)

$(TARGET): enum_devices.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Built $(TARGET) successfully"

clean:
	rm -f $(TARGET) *.o

# For native Windows compilation:
# cl /W4 /EHsc /O2 enum_devices.cpp /Fe:enum_devices.exe /link setupapi.lib cfgmgr32.lib
