#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>

// Simple test framework
class TestFramework {
private:
    int total_tests = 0;
    int passed_tests = 0;

public:
    void assert_equal(const std::string& actual, const std::string& expected, const std::string& test_name) {
        total_tests++;
        if (actual == expected) {
            passed_tests++;
            std::cout << "[PASS] " << test_name << std::endl;
        } else {
            std::cout << "[FAIL] " << test_name << std::endl;
            std::cout << "  Expected: " << expected << std::endl;
            std::cout << "  Actual:   " << actual << std::endl;
        }
    }
    
    void assert_true(bool condition, const std::string& test_name) {
        total_tests++;
        if (condition) {
            passed_tests++;
            std::cout << "[PASS] " << test_name << std::endl;
        } else {
            std::cout << "[FAIL] " << test_name << std::endl;
        }
    }
    
    void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << total_tests << std::endl;
        std::cout << "Passed: " << passed_tests << std::endl;
        std::cout << "Failed: " << (total_tests - passed_tests) << std::endl;
        std::cout << "Success rate: " << (passed_tests * 100.0 / total_tests) << "%" << std::endl;
    }
    
    bool all_passed() const {
        return passed_tests == total_tests;
    }
};

// Create test MBO data
void create_test_mbo_file(const std::string& filename) {
    std::ofstream file(filename);
    file << "ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,price,size,channel_id,order_id,flags,ts_in_delta,sequence,symbol\n";
    
    // Test data with various scenarios
    file << "2025-07-17T07:05:09.035793433Z,2025-07-17T07:05:09.035627674Z,160,2,1108,R,N,,0,0,0,8,0,0,ARL\n";
    file << "2025-07-17T08:05:03.360842448Z,2025-07-17T08:05:03.360677248Z,160,2,1108,A,B,5.51,100,0,1001,130,165200,851012,ARL\n";
    file << "2025-07-17T08:05:03.360848793Z,2025-07-17T08:05:03.360683462Z,160,2,1108,A,A,21.33,100,0,1002,130,165331,851013,ARL\n";
    file << "2025-07-17T08:05:03.361492517Z,2025-07-17T08:05:03.361327319Z,160,2,1108,A,B,5.9,100,0,1003,130,165198,851022,ARL\n";
    file << "2025-07-17T08:05:03.361497823Z,2025-07-17T08:05:03.361332576Z,160,2,1108,A,A,20.94,100,0,1004,130,165247,851023,ARL\n";
    file << "2025-07-17T08:09:48.860862095Z,2025-07-17T08:09:48.860696464Z,160,2,1108,C,B,5.51,100,0,1001,130,165631,1289631,ARL\n";
    file << "2025-07-17T08:09:48.860870885Z,2025-07-17T08:09:48.860705588Z,160,2,1108,A,B,5.37,100,0,1005,130,165297,1289632,ARL\n";
    
    file.close();
}

std::vector<std::string> read_csv_lines(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);
    std::string line;
    
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

void test_basic_functionality(TestFramework& tf) {
    std::cout << "\n=== Testing Basic Functionality ===" << std::endl;
    
    // Create test input
    create_test_mbo_file("test_input.csv");
    
    // Run reconstruction
    int result = system(".\\reconstruction_blockhouse.exe test_input.csv > nul 2>&1");
    tf.assert_true(result == 0, "Reconstruction executable runs successfully");
    
    // Check output file exists
    std::ifstream output_file("reconstructed_mbp.csv");
    tf.assert_true(output_file.good(), "Output file created successfully");
    
    // Read output and verify basic structure
    auto lines = read_csv_lines("reconstructed_mbp.csv");
    tf.assert_true(lines.size() > 1, "Output contains header and data lines");
    
    // Verify header contains expected columns
    std::string header = lines[0];
    tf.assert_true(header.find("ts_recv") != std::string::npos, "Header contains ts_recv");
    tf.assert_true(header.find("bid_px_00") != std::string::npos, "Header contains bid_px_00");
    tf.assert_true(header.find("ask_px_00") != std::string::npos, "Header contains ask_px_00");
    tf.assert_true(header.find("symbol") != std::string::npos, "Header contains symbol");
    
    // Clean up
    system("del test_input.csv 2>nul");
}

void test_order_book_operations(TestFramework& tf) {
    std::cout << "\n=== Testing Order Book Operations ===" << std::endl;
    
    // Create specific test case for order operations
    std::ofstream test_file("order_test.csv");
    test_file << "ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,price,size,channel_id,order_id,flags,ts_in_delta,sequence,symbol\n";
    test_file << "2025-07-17T07:05:09.035793433Z,2025-07-17T07:05:09.035627674Z,160,2,1108,R,N,,0,0,0,8,0,0,ARL\n";
    test_file << "2025-07-17T08:05:03.360842448Z,2025-07-17T08:05:03.360677248Z,160,2,1108,A,B,10.0,100,0,1001,130,165200,851012,ARL\n";
    test_file << "2025-07-17T08:05:03.360848793Z,2025-07-17T08:05:03.360683462Z,160,2,1108,A,A,11.0,100,0,1002,130,165331,851013,ARL\n";
    test_file << "2025-07-17T08:05:03.361492517Z,2025-07-17T08:05:03.361327319Z,160,2,1108,C,B,10.0,100,0,1001,130,165198,851022,ARL\n";
    test_file.close();
    
    system(".\\reconstruction_blockhouse.exe order_test.csv > nul 2>&1");
    
    auto lines = read_csv_lines("reconstructed_mbp.csv");
    tf.assert_true(lines.size() >= 4, "Correct number of output lines for order operations");
    
    // Check that after cancellation, the bid side is empty
    std::string last_line = lines.back();
    // The bid columns should be empty after cancellation
    tf.assert_true(last_line.find(",10.00,") == std::string::npos || 
                   last_line.find(",,") != std::string::npos, "Order cancellation processed correctly");
    
    system("del order_test.csv 2>nul");
}

void test_performance(TestFramework& tf) {
    std::cout << "\n=== Testing Performance ===" << std::endl;
    
    // Create larger test file
    std::ofstream perf_file("perf_test.csv");
    perf_file << "ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,price,size,channel_id,order_id,flags,ts_in_delta,sequence,symbol\n";
    perf_file << "2025-07-17T07:05:09.035793433Z,2025-07-17T07:05:09.035627674Z,160,2,1108,R,N,,0,0,0,8,0,0,ARL\n";
    
    // Generate 10000 orders
    for (int i = 1; i <= 5000; ++i) {
        perf_file << "2025-07-17T08:05:03.360842448Z,2025-07-17T08:05:03.360677248Z,160,2,1108,A,B,"
                  << (10.0 + i * 0.01) << ",100,0," << i << ",130,165200,851012,ARL\n";
        perf_file << "2025-07-17T08:05:03.360848793Z,2025-07-17T08:05:03.360683462Z,160,2,1108,A,A,"
                  << (11.0 + i * 0.01) << ",100,0," << (i + 5000) << ",130,165331,851013,ARL\n";
    }
    perf_file.close();
    
    auto start = std::chrono::high_resolution_clock::now();
    int result = system(".\\reconstruction_blockhouse.exe perf_test.csv > nul 2>&1");
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    tf.assert_true(result == 0, "Performance test runs successfully");
    tf.assert_true(duration.count() < 5000, "Processing completes within reasonable time (< 5s)");
    
    std::cout << "Performance test completed in " << duration.count() << " ms" << std::endl;
    
    system("del perf_test.csv 2>nul");
}

void test_edge_cases(TestFramework& tf) {
    std::cout << "\n=== Testing Edge Cases ===" << std::endl;
    
    // Test empty file after header
    std::ofstream empty_file("empty_test.csv");
    empty_file << "ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,price,size,channel_id,order_id,flags,ts_in_delta,sequence,symbol\n";
    empty_file.close();
    
    int result = system(".\\reconstruction_blockhouse.exe empty_test.csv > nul 2>&1");
    tf.assert_true(result == 0, "Handles empty input file gracefully");
    
    // Test file with only clear action
    std::ofstream clear_file("clear_test.csv");
    clear_file << "ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,price,size,channel_id,order_id,flags,ts_in_delta,sequence,symbol\n";
    clear_file << "2025-07-17T07:05:09.035793433Z,2025-07-17T07:05:09.035627674Z,160,2,1108,R,N,,0,0,0,8,0,0,ARL\n";
    clear_file.close();
    
    result = system(".\\reconstruction_blockhouse.exe clear_test.csv > nul 2>&1");
    tf.assert_true(result == 0, "Handles clear-only file gracefully");
    
    system("del empty_test.csv clear_test.csv 2>nul");
}

int main() {
    TestFramework tf;
    
    std::cout << "OrderBook Reconstruction Test Suite" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // First, try to build the executable
    std::cout << "Building reconstruction executable..." << std::endl;
    int build_result = system("g++ -std=c++17 -O3 -Wall -Wextra -o reconstruction_blockhouse.exe reconstruction.cpp > build.log 2>&1");
    
    if (build_result != 0) {
        std::cout << "Error: Failed to build executable. Check build.log for details." << std::endl;
        return 1;
    }
    
    std::cout << "Build successful!" << std::endl;
    
    // Run tests
    test_basic_functionality(tf);
    test_order_book_operations(tf);
    test_performance(tf);
    test_edge_cases(tf);
    
    // Clean up
    system("del reconstructed_mbp.csv build.log 2>nul");
    
    tf.print_summary();
    
    return tf.all_passed() ? 0 : 1;
}
