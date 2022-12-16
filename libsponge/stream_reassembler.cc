#include "stream_reassembler.hh"

#include <iostream>
#include <string>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void StreamReassembler::addBlock(const size_t idx, const size_t len) {
    for (size_t i = idx; i < idx + len; i++) {
        vis[i] = true;
    }
}
StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    // if(eof) _eof = true
    size_t _index = index;
    string inputStr = "";
    size_t remain = _output.remaining_capacity();
    // size_t dataSize = data.size();
    if (index > posOfFin + remain)
        return;
    if (index < posOfFin) {
        if (index + data.size() < posOfFin)
            return;
        else {
            inputStr = data.substr(posOfFin - index, remain);
            _index = posOfFin;
        }
    } else {
        inputStr = data;
    }
    if (_index + inputStr.size() > posOfFin + remain) {
        inputStr = inputStr.substr(0, posOfFin + remain - _index);
    }

    // nowInFlow += inputStr.size();
    if (eof) {
        _eof = true;
        endIndex = index + data.size();
    }
    // vis.push_back({_index, inputStr.size()});
    StreamReassembler::addBlock(_index, inputStr.size());
    flow.push({_index, inputStr});

    while (!flow.empty()) {
        block pices = flow.top();
        if (posOfFin == pices.index) {
            _output.write(pices.str);
            posOfFin += pices.str.size();
            // nowInFlow -= pices.str.size();
            flow.pop();
        } else if (pices.index < posOfFin) {
            if (pices.index + pices.str.size() < posOfFin) {
                //  nowInFlow -= pices.str.size();
                flow.pop();
            } else {
                // nowInFlow -= pices.str.size();
                pices.str.erase(0, posOfFin - pices.index);
                posOfFin += pices.str.size();
                _output.write(pices.str);
                // nowInFlow -= pices.str.size();
                flow.pop();
            }
        } else
            break;
    }

    if (empty() && _eof) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() { return countBlock(); }

bool StreamReassembler::empty() const { return posOfFin == endIndex; }

size_t StreamReassembler::countBlock() {
    size_t sum = 0;
    for (auto i : vis) {
        if (i.first > posOfFin)
            sum++;
    }
    return sum;
}
