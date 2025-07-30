#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <chrono>

struct OrderBookLevel {
    double price;
    int size;
    int count;
    
    OrderBookLevel() : price(0.0), size(0), count(0) {}
    OrderBookLevel(double p, int s, int c) : price(p), size(s), count(c) {}
};

struct Order {
    int order_id;
    char side;
    double price;
    int size;
    
    Order() : order_id(0), side('N'), price(0.0), size(0) {}
    Order(int id, char s, double p, int sz) : order_id(id), side(s), price(p), size(sz) {}
};

class OrderBook {
private:
    std::map<double, OrderBookLevel, std::greater<double>> bids;  // Descending order
    std::map<double, OrderBookLevel> asks;                       // Ascending order
    std::unordered_map<int, Order> orders;                      // order_id -> Order
    
public:
    void addOrder(int order_id, char side, double price, int size) {
        orders[order_id] = Order(order_id, side, price, size);
        
        if (side == 'B') {
            bids[price].price = price;
            bids[price].size += size;
            bids[price].count++;
        } else if (side == 'A') {
            asks[price].price = price;
            asks[price].size += size;
            asks[price].count++;
        }
    }
    
    void cancelOrder(int order_id) {
        auto it = orders.find(order_id);
        if (it == orders.end()) return;
        
        const Order& order = it->second;
        if (order.side == 'B') {
            auto bid_it = bids.find(order.price);
            if (bid_it != bids.end()) {
                bid_it->second.size -= order.size;
                bid_it->second.count--;
                if (bid_it->second.size <= 0) {
                    bids.erase(bid_it);
                }
            }
        } else if (order.side == 'A') {
            auto ask_it = asks.find(order.price);
            if (ask_it != asks.end()) {
                ask_it->second.size -= order.size;
                ask_it->second.count--;
                if (ask_it->second.size <= 0) {
                    asks.erase(ask_it);
                }
            }
        }
        
        orders.erase(it);
    }
    
    void modifyOrder(int order_id, double new_price, int new_size) {
        cancelOrder(order_id);
        auto it = orders.find(order_id);
        if (it != orders.end()) {
            addOrder(order_id, it->second.side, new_price, new_size);
        }
    }
    
    void clear() {
        bids.clear();
        asks.clear();
        orders.clear();
    }
    
    std::vector<OrderBookLevel> getBids(int depth = 10) const {
        std::vector<OrderBookLevel> result;
        auto it = bids.begin();
        for (int i = 0; i < depth && it != bids.end(); ++i, ++it) {
            result.push_back(it->second);
        }
        return result;
    }
    
    std::vector<OrderBookLevel> getAsks(int depth = 10) const {
        std::vector<OrderBookLevel> result;
        auto it = asks.begin();
        for (int i = 0; i < depth && it != asks.end(); ++i, ++it) {
            result.push_back(it->second);
        }
        return result;
    }
};

struct MBORecord {
    std::string ts_recv;
    std::string ts_event;
    int rtype;
    int publisher_id;
    int instrument_id;
    char action;
    char side;
    double price;
    int size;
    int channel_id;
    int order_id;
    int flags;
    int ts_in_delta;
    int sequence;
    std::string symbol;
};

class CSVParser {
public:
    static std::vector<std::string> parseLine(const std::string& line) {
        std::vector<std::string> result;
        std::stringstream ss(line);
        std::string field;
        
        while (std::getline(ss, field, ',')) {
            result.push_back(field);
        }
        
        return result;
    }
    
    static MBORecord parseMBORecord(const std::vector<std::string>& fields) {
        MBORecord record;
        
        if (fields.size() >= 15) {
            record.ts_recv = fields[0];
            record.ts_event = fields[1];
            record.rtype = std::stoi(fields[2]);
            record.publisher_id = std::stoi(fields[3]);
            record.instrument_id = std::stoi(fields[4]);
            record.action = fields[5].empty() ? 'N' : fields[5][0];
            record.side = fields[6].empty() ? 'N' : fields[6][0];
            record.price = fields[7].empty() ? 0.0 : std::stod(fields[7]);
            record.size = fields[8].empty() ? 0 : std::stoi(fields[8]);
            record.channel_id = std::stoi(fields[9]);
            record.order_id = std::stoi(fields[10]);
            record.flags = std::stoi(fields[11]);
            record.ts_in_delta = std::stoi(fields[12]);
            record.sequence = std::stoi(fields[13]);
            record.symbol = fields[14];
        }
        
        return record;
    }
};

class OrderBookReconstructor {
private:
    OrderBook orderbook;
    std::ofstream output_file;
    int row_index;
    
    // Track pending trades for T->F->C sequence
    struct PendingTrade {
        std::string ts_recv;
        std::string ts_event;
        int rtype;
        int publisher_id;
        int instrument_id;
        char actual_side;  // The side that should be affected in the book
        double price;
        int size;
        int flags;
        int ts_in_delta;
        int sequence;
        std::string symbol;
        int order_id;
    };
    
    std::vector<PendingTrade> pending_trades;
    
public:
    OrderBookReconstructor(const std::string& output_filename) 
        : output_file(output_filename), row_index(0) {
        writeHeader();
    }
    
    ~OrderBookReconstructor() {
        if (output_file.is_open()) {
            output_file.close();
        }
    }
    
    void writeHeader() {
        output_file << ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence";
        
        // Write bid/ask columns for 10 levels
        for (int i = 0; i < 10; ++i) {
            output_file << ",bid_px_" << std::setfill('0') << std::setw(2) << i
                       << ",bid_sz_" << std::setfill('0') << std::setw(2) << i
                       << ",bid_ct_" << std::setfill('0') << std::setw(2) << i
                       << ",ask_px_" << std::setfill('0') << std::setw(2) << i
                       << ",ask_sz_" << std::setfill('0') << std::setw(2) << i
                       << ",ask_ct_" << std::setfill('0') << std::setw(2) << i;
        }
        
        output_file << ",symbol,order_id\n";
    }
    
    void writeMBPRecord(const MBORecord& record, char effective_action, char effective_side, int depth) {
        auto bids = orderbook.getBids(10);
        auto asks = orderbook.getAsks(10);
        
        output_file << row_index << "," << record.ts_event << "," << record.ts_event << ","
                   << "10" << "," << record.publisher_id << "," << record.instrument_id << ","
                   << effective_action << "," << effective_side << "," << depth << ",";
        
        if (record.price > 0) {
            output_file << std::fixed << std::setprecision(8) << record.price;
        }
        output_file << "," << record.size << "," << record.flags << ","
                   << record.ts_in_delta << "," << record.sequence;
        
        // Write 10 levels of bid/ask data
        for (int i = 0; i < 10; ++i) {
            if (i < static_cast<int>(bids.size())) {
                output_file << "," << std::fixed << std::setprecision(2) << bids[i].price
                           << "," << bids[i].size << "," << bids[i].count;
            } else {
                output_file << ",,0,0";
            }
            
            if (i < static_cast<int>(asks.size())) {
                output_file << "," << std::fixed << std::setprecision(2) << asks[i].price
                           << "," << asks[i].size << "," << asks[i].count;
            } else {
                output_file << ",,0,0";
            }
        }
        
        output_file << "," << record.symbol << "," << record.order_id << "\n";
        row_index++;
    }
    
    void processRecord(const MBORecord& record) {
        // Skip the initial clear record
        if (record.action == 'R') {
            orderbook.clear();
            writeMBPRecord(record, 'R', 'N', 0);
            return;
        }
        
        // Handle Trade actions with special logic
        if (record.action == 'T') {
            // If side is 'N', don't alter the orderbook
            if (record.side == 'N') {
                return;
            }
            
            // Store pending trade - we'll process it when we see the corresponding F and C
            PendingTrade trade;
            trade.ts_recv = record.ts_recv;
            trade.ts_event = record.ts_event;
            trade.rtype = record.rtype;
            trade.publisher_id = record.publisher_id;
            trade.instrument_id = record.instrument_id;
            trade.price = record.price;
            trade.size = record.size;
            trade.flags = record.flags;
            trade.ts_in_delta = record.ts_in_delta;
            trade.sequence = record.sequence;
            trade.symbol = record.symbol;
            trade.order_id = record.order_id;
            trade.sequence = record.sequence;  // Track by sequence number
            
            // The actual side affected is opposite to the trade side
            trade.actual_side = (record.side == 'B') ? 'A' : 'B';
            
            pending_trades.push_back(trade);
            return;
        }
        
        // Handle Fill actions - just store, don't process yet
        if (record.action == 'F') {
            return;
        }
        
        // Handle Cancel actions
        if (record.action == 'C') {
            // Check if this cancel is part of a T->F->C sequence
            bool is_trade_cancel = false;
            for (auto it = pending_trades.begin(); it != pending_trades.end(); ++it) {
                if (it->sequence == record.sequence) {
                    // This is a trade cancel - process the trade effect
                    orderbook.cancelOrder(record.order_id);
                    
                    // Write the trade record with the correct side
                    MBORecord trade_record = record;
                    trade_record.action = 'T';
                    trade_record.side = it->actual_side;
                    trade_record.price = it->price;
                    trade_record.size = it->size;
                    
                    int depth = 0;
                    if (it->actual_side == 'B') {
                        auto bids = orderbook.getBids(10);
                        for (int i = 0; i < static_cast<int>(bids.size()); ++i) {
                            if (bids[i].price == it->price) {
                                depth = i;
                                break;
                            }
                        }
                    } else {
                        auto asks = orderbook.getAsks(10);
                        for (int i = 0; i < static_cast<int>(asks.size()); ++i) {
                            if (asks[i].price == it->price) {
                                depth = i;
                                break;
                            }
                        }
                    }
                    
                    writeMBPRecord(trade_record, 'T', it->actual_side, depth);
                    
                    pending_trades.erase(it);
                    is_trade_cancel = true;
                    break;
                }
            }
            
            if (!is_trade_cancel) {
                // Regular cancel
                int depth = 0;
                if (record.side == 'B') {
                    auto bids = orderbook.getBids(10);
                    for (int i = 0; i < static_cast<int>(bids.size()); ++i) {
                        if (bids[i].price == record.price) {
                            depth = i;
                            break;
                        }
                    }
                } else if (record.side == 'A') {
                    auto asks = orderbook.getAsks(10);
                    for (int i = 0; i < static_cast<int>(asks.size()); ++i) {
                        if (asks[i].price == record.price) {
                            depth = i;
                            break;
                        }
                    }
                }
                
                orderbook.cancelOrder(record.order_id);
                writeMBPRecord(record, 'C', record.side, depth);
            }
            return;
        }
        
        // Handle Add actions
        if (record.action == 'A') {
            orderbook.addOrder(record.order_id, record.side, record.price, record.size);
            
            int depth = 0;
            if (record.side == 'B') {
                auto bids = orderbook.getBids(10);
                for (int i = 0; i < static_cast<int>(bids.size()); ++i) {
                    if (bids[i].price == record.price) {
                        depth = i;
                        break;
                    }
                }
            } else if (record.side == 'A') {
                auto asks = orderbook.getAsks(10);
                for (int i = 0; i < static_cast<int>(asks.size()); ++i) {
                    if (asks[i].price == record.price) {
                        depth = i;
                        break;
                    }
                }
            }
            
            writeMBPRecord(record, 'A', record.side, depth);
            return;
        }
        
        // Handle Modify actions
        if (record.action == 'M') {
            orderbook.modifyOrder(record.order_id, record.price, record.size);
            writeMBPRecord(record, 'M', record.side, 0);
            return;
        }
    }
    
    void processFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return;
        }
        
        std::string line;
        bool first_line = true;
        
        while (std::getline(file, line)) {
            if (first_line) {
                first_line = false;
                continue; // Skip header
            }
            
            auto fields = CSVParser::parseLine(line);
            if (fields.size() >= 15) {
                MBORecord record = CSVParser::parseMBORecord(fields);
                processRecord(record);
            }
        }
        
        file.close();
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mbo_input_file.csv>" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = "reconstructed_mbp.csv";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    OrderBookReconstructor reconstructor(output_file);
    reconstructor.processFile(input_file);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Order book reconstruction completed in " << duration.count() << " ms" << std::endl;
    std::cout << "Output written to: " << output_file << std::endl;
    
    return 0;
}
