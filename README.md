# OrderBook Reconstruction System

C++ implementation that reconstructs MBP-10 order book data from MBO market data. Built for the BlockHouse quantitative developer work trial.

## What it does

Takes market-by-order (MBO) data and reconstructs the market-by-price (MBP-10) order book view. Handles all the tricky edge cases like T→F→C sequences and maintains proper price-time priority.

## Getting started

**Windows:**
```bash
.\build.bat run
```

**Linux/Mac:**
```bash
make && ./reconstruction_blockhouse mbo.csv
```

**Testing:**
```bash
.\build.bat test     # Windows
make run_tests       # Linux/Mac
```

## Key features

**Special operations handled:**
- Ignores the initial clear[R] action (starts with empty book)
- Properly processes T→F→C sequences as single trade events
- Correctly identifies which side of the book gets affected by trades
- Filters out trades with side 'N' as specified

**Performance stuff:**
- Uses std::map for O(log n) operations on price levels
- Single-pass streaming (doesn't load everything into memory)
- Minimal allocations during processing
- Fast enough for HFT requirements

## How it works

The core is an `OrderBook` class that maintains sorted bid/ask levels. The `OrderBookReconstructor` processes each MBO record and updates the book state accordingly. 

For T→F→C sequences, I store the trade info when seeing 'T', ignore the 'F', then process everything when the 'C' comes through. The trade side logic was tricky - if you see a trade on the ask side, it actually removes from the bid side (since that's where the resting order was).

Output format matches the reference mbp.csv exactly.

## Building

**Requirements:**
- C++17 compiler (g++ 7.0+, Visual Studio 2017+)
- No external dependencies

**Build commands:**
```bash
# Manual build
g++ -std=c++17 -O3 -Wall -Wextra -o reconstruction_blockhouse reconstruction.cpp

# Or use the provided scripts
.\build.bat        # Windows
make              # Linux/Mac
```

## Usage

```bash
./reconstruction_blockhouse mbo.csv
```

Creates `reconstructed_mbp.csv` with the MBP-10 data. Also prints timing info.

## Project files

```
reconstruction.cpp    # Main implementation
test_suite.cpp        # Test cases
Makefile              # Linux build
build.bat             # Windows build  
mbo.csv               # Sample input
mbp.csv               # Expected output
```

## Performance notes

Processes the sample dataset in under a second. Uses balanced trees for the price levels so operations stay fast even with deep books. Memory usage is proportional to the number of active orders and price levels.

The trickiest part was getting the T→F→C sequence handling right. Initially tried matching by order_id but that doesn't work for trades (order_id is 0). Switched to matching by sequence number which fixed it.

## Tests

Run `.\build.bat test` to execute the test suite. Covers basic functionality, order operations, performance, and edge cases.
