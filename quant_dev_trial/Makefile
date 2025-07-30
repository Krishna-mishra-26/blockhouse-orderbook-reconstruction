CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native
TARGET = reconstruction_blockhouse
SOURCE = reconstruction.cpp
TEST_TARGET = test_suite
TEST_SOURCE = test_suite.cpp

.PHONY: all clean test run_tests

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

$(TEST_TARGET): $(TEST_SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SOURCE)

clean:
	rm -f $(TARGET) $(TEST_TARGET) reconstructed_mbp.csv *.log

test: $(TARGET)
	./$(TARGET) mbo.csv

run_tests: $(TEST_TARGET)
	./$(TEST_TARGET)

test: $(TARGET)
	./$(TARGET) mbo.csv

benchmark: $(TARGET)
	time ./$(TARGET) mbo.csv

profile: reconstruction.cpp
	$(CXX) $(CXXFLAGS) -pg -o $(TARGET)_profile $(SOURCE)
	./$(TARGET)_profile mbo.csv
	gprof $(TARGET)_profile gmon.out > profile_analysis.txt

help:
	@echo "Available targets:"
	@echo "  all       - Build the reconstruction executable"
	@echo "  clean     - Remove built files and output"
	@echo "  test      - Build and run with mbo.csv"
	@echo "  run_tests - Build and run comprehensive test suite"
	@echo "  benchmark - Build and run with timing"
	@echo "  profile   - Build with profiling enabled"
	@echo "  help      - Show this help message"
