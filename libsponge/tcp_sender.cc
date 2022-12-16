#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return flightBytes; }

void TCPSender::fill_window() {
    if (FINFlag) {
        return;
    }
    size_t curr_window_size = _windowSize ? _windowSize : 1;
    while (curr_window_size > flightBytes) {
        TCPSegment seg;
        // if not send syn yet, set it
        if (!SYNFlag) {
            SYNFlag = true;
            seg.header().syn = true;
        }

        // set seqno
        seg.header().seqno = next_seqno();

        size_t payload_Size = min(TCPConfig::MAX_PAYLOAD_SIZE, curr_window_size - flightBytes - seg.header().syn);
        string payload = _stream.read(payload_Size);

        if (_stream.eof() && !FINFlag && payload.size() + flightBytes < curr_window_size) {
            FINFlag = true;
            seg.header().fin = true;
        }

        seg.payload() = Buffer(move(payload));

        if (seg.length_in_sequence_space() == 0) {
            break;
        }

        if (_outstandingSegment.empty()) {
            _timer.run = true;
            _retransmission_timeout = _initial_retransmission_timeout;
            _timer.tick = 0;
        }

        _segments_out.push(seg);
        _outstandingSegment.push(seg);

        flightBytes += seg.length_in_sequence_space();
        _next_seqno += seg.length_in_sequence_space();

        if (FINFlag) {
            break;
        }
    }

    return;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t absAckno = unwrap(ackno, _isn, _receivedAckno);
    // if this absAckno is wrong, return
    if (absAckno > _next_seqno)
        return;
    _windowSize = window_size;

    // if this absAckno is old, return
    if (absAckno <= _receivedAckno)
        return;
    else
        _receivedAckno = absAckno;

    // pop arrived segments
    while (!_outstandingSegment.empty()) {
        TCPSegment _seg = _outstandingSegment.front();

        // if the seqno at the end of outstanding segment is less than or equal to absAckno,
        // pop it, it means this segment has been received
        if (unwrap(_seg.header().seqno, _isn, _receivedAckno) + _seg.length_in_sequence_space() <= absAckno) {
            flightBytes -= _seg.length_in_sequence_space();

            _outstandingSegment.pop();
        } else
            break;
    }

    // fill window in time

    fill_window();

    // handle something mentioned in Lab3 (reset the timer)
    _retransmission_timeout = _initial_retransmission_timeout;
    _numberOfRetransmission = 0;
    _timer.tick = 0;
    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.tick += ms_since_last_tick;

    // Enter if timer has started && there are segments flying && time out,
    // then we resend the unresponsive segment
    if (_timer.run && !_outstandingSegment.empty() && _timer.tick >= _retransmission_timeout) {
        // step 1 : retransmission
        _segments_out.push(_outstandingSegment.front());
        // step 2 : if window size is nonzero, track it and double the RTO
        // BUT syn segment still need this
        if (_windowSize != 0 || _outstandingSegment.front().header().syn) {
            _numberOfRetransmission++;
            _retransmission_timeout *= 2;
        }
        // step 3 : reset the timer
        _timer.tick = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _numberOfRetransmission; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
    return;
}

void TCPSender::send_RST_Segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    seg.header().rst = true;
    _segments_out.push(seg);
    return;
}
