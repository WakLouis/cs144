#include "tcp_connection.hh"

#include <cassert>
#include <iostream>
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment = 0;

    bool need_ack = seg.length_in_sequence_space();

    _receiver.segment_received(seg);

    // 收到RST
    if (seg.header().rst) {
        set_rst_state(false);
        return;
    }

    // 收到ACK
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        if (need_ack && !_sender.segments_out().empty()) {
            need_ack = false;
        }
    }

    if (!_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0) {
        connect();
        return;
    }

    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_ACKED) {
        _linger_after_streams_finish = false;
    }

    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && !_linger_after_streams_finish) {
        _active = false;
        return;
    }

    if (need_ack) {
        _sender.send_empty_segment();
    }

    add_Ackno_And_Win_And_Push();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t len = _sender.stream_in().write(data);
    _sender.fill_window();
    add_Ackno_And_Win_And_Push();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        set_rst_state(true);
        return;
    }

    _time_since_last_segment += ms_since_last_tick;

    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && _linger_after_streams_finish &&
        _time_since_last_segment >= 10 * _cfg.rt_timeout) {
        _active = false;
        _linger_after_streams_finish = false;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    add_Ackno_And_Win_And_Push();
    return;
}

void TCPConnection::connect() {
    _sender.fill_window();
    add_Ackno_And_Win_And_Push();
    _active = true;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            set_rst_state(false);
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::add_Ackno_And_Win_And_Push() {
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);
    }
}

void TCPConnection::set_rst_state(bool send_rst) {
    if (send_rst) {
        while (!_segments_out.empty()) {
            _segments_out.pop();
        }
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.push(seg);
    }
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _linger_after_streams_finish = false;
    _active = false;
}
