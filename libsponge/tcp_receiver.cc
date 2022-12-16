#include "tcp_receiver.hh"

#include <cassert>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

uint32_t TCPReceiver::SynOrFin() const { return (stream_out().input_ended() ? 1 : 0); }

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn == false && synFlag == false) {
        return;
    }

    if (seg.header().syn == true) {
        if (synFlag) {
            return;
        } else {
            synFlag = true;
            _isn = seg.header().seqno;
        }
    }

    if (seg.header().fin == true) {
        finFlag = true;
    }

    if (synFlag) {
        size_t index = unwrap(seg.header().seqno, _isn.value(), stream_out().bytes_written()) - 1;
        if (seg.header().syn)
            index = 0;

        _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);
    }

    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn.has_value()) {
        return nullopt;
    }

    return wrap(stream_out().bytes_written() + 1 + SynOrFin(), _isn.value());
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
