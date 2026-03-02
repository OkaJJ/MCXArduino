CXX := g++
CXXFLAGS := -O3 -Wall -Wextra

SRC := src/mcx-discovery.cpp
OUT := mcx-discovery

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@
	chmod +x $@

clean:
	rm -f $(OUT)
