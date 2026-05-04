CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I.
TARGET   = cricket
SRC      = main.cpp

all: $(TARGET)

$(TARGET): $(SRC) include/algorithms.h include/models.h include/ball_generator.h include/match_engine.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
