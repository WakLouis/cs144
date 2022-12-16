#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { _capacity = capacity; }

size_t ByteStream::write(const string &data) {
    size_t len = data.length();
    size_t remain = remaining_capacity();
    size_t writeNum;
    if (len > remain) {
        _stream += data.substr(0, remain);
        writeNum = remain;
    } else {
        _stream += data;
        writeNum = len;
    }
    _numberOfWritten += writeNum;
    return writeNum;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t num;
    if (_stream.length() < len)
        num = _stream.length();
    else
        num = len;
    string peekstring = _stream.substr(0, num);
    return peekstring;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t num;
    if (len > _stream.length())
        num = _stream.length();
    else
        num = len;
    _stream.erase(_stream.begin(), _stream.begin() + num);
    _numberOfRead += num;
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string readstring = peek_output(len);
    pop_output(len);
    return readstring;
}

void ByteStream::end_input() { _isEnd = true; }

bool ByteStream::input_ended() const { return _isEnd; }

size_t ByteStream::buffer_size() const { return _stream.length(); }

bool ByteStream::buffer_empty() const { return _stream.empty(); }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _numberOfWritten; }

size_t ByteStream::bytes_read() const { return _numberOfRead; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
