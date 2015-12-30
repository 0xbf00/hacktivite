#include <iostream>
#include <fstream>
#include <iomanip>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cstdint>
#include <cstddef>
#include <cstdlib>

#include "result_queue.h"

// Clear flags of the input stream and seek to the beginning
// clearing the flags is neccessary because the failbit could be set,
// if the file is not aligned to 4 bytes!
void seek_start(std::ifstream &in)
{
    in.clear();
    in.seekg(0, std::ios::beg);
}

// Seek to the end of the input stream
void seek_end(std::ifstream &in)
{
    in.seekg(0, std::ios::end);
}

// Obtain the file size in bytes
uint32_t file_get_size(std::ifstream &in)
{
    seek_end(in);
    auto size = in.tellg();
    seek_start(in);
    return size;
}

/* Type that holds the count for each pointer candidate:
   if a pointer occurs more than once, the corresponding
   count is incremented */
using ptr_map = std::unordered_map<uint32_t, std::size_t>;

// Returns a map containing the found pointers (uint32_t), only for 32-bit binaries
// and the associated count, that is, how many times the specific pointer occured.
ptr_map get_pointers(std::ifstream &in)
{
    seek_start(in);
    
    ptr_map ret;
    
    while (in) {
        uint32_t v = 0;
        in.read(reinterpret_cast<char*>(&v), sizeof(v));
        // If the file is not 4-byte aligned, we might set the failbit...
        if (!in.fail())
            ret[v]++;
    }
    
    return ret;
}

// Contains the address of each string found for the binary
using stroff_set = std::unordered_set<uint32_t>;

/* Returns a set with the offsets of found strings.
   The additional parameter n can be used to change the minimum length
   of a string */
stroff_set get_strings(std::ifstream &in, std::string::size_type n = 4)
{
    seek_start(in);
    
    std::istreambuf_iterator<char> sbi(in), eof;
    std::string contents(sbi, eof);
    stroff_set results;
    
    const char wordset[] =  "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "0123456789"
                            "[]()<>/*=_.,:+-'\"%$! ";
    
    std::string searching(wordset, strlen(wordset));

    std::string::size_type start_pos = 0;
    
    /*
     * Manual searching is used. Even if this is inefficient, this function is only
     * called once and is not on the hot path. There is no point in optimizing it!
     */
    while (start_pos != std::string::npos) {
        start_pos = contents.find_first_of(searching, start_pos);
        auto end_pos = contents.find_first_not_of(searching, start_pos);
        
        if (start_pos != std::string::npos &&
            end_pos   != std::string::npos &&
            (end_pos - start_pos) >= n)
        {
            std::cout << "Found string: " << std::string(contents, start_pos, end_pos - start_pos) << std::endl;
            results.insert(start_pos);
        }
    
        start_pos = end_pos;
    }
    
    return results;
}

using result = std::pair<uint32_t, std::size_t>;

template<> struct std::greater<result>
{
    bool operator()(const result &lhs, const result &rhs) {
        return lhs.second > rhs.second;
    }
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "[Usage]: " << argv[0] << " binary" << "\n";
        return EXIT_FAILURE;
    }
    
    constexpr uint32_t end_base = 0xf0000000;
        
    std::ifstream in(argv[1]);
    if (in) {
        const auto filesize     = file_get_size(in);
        auto pointers_ret       = get_pointers(in);
        const auto strings_ret  = get_strings(in);
        
        result_queue<result, std::greater<result>> scores(20);
        result                                     best_score{0,0};
    
        for (uint32_t base = 0x0; base <= end_base; base += 0x1000)
        {
            // "Live" updates
            if (base % 0x10000 == 0) {
                std::cout   << "\r["    << std::fixed << std::setprecision(2)
                                        << (100.0 * (static_cast<double>(base) / end_base))
                            << "%] Current best match is " << std::hex << std::showbase << best_score.first << "\r"
                            << std::noshowbase << std::dec;
            }
        
            std::size_t score{0};

            for (auto beg = pointers_ret.begin(); beg != pointers_ret.end(); ++beg) {
                uint32_t ptr{beg->first};
                if (ptr < base) {
                    beg = pointers_ret.erase(beg);
                    if (beg == pointers_ret.end())
                        break;
                    continue;
                } else if (ptr >= (base + filesize)) {
                    continue;
                }
        
                uint32_t offset = ptr - base;
                if (strings_ret.find(offset) != strings_ret.end()) {
                    score += beg->second;
                }
            }

            if (score > best_score.second) {
                best_score = std::make_pair(base, score);    
            }

            scores.addResult(std::make_pair(base, score));
        }        
        

        // Print best matches
        scores.printResults([](const result &r) {
            std::cout << "Base address " << std::showbase << std::hex << std::setw(8) << std::setfill('0') << r.first 
                      << " with " << std::noshowbase      << std::dec << r.second << " matches." << std::endl;
        });
    }
}
