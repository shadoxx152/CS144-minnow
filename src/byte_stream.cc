#include "byte_stream.hh"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <sys/types.h>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_( capacity ) {}

void Writer::push( string data )
{
	uint64_t size_of_write = std::min( data.size(), Writer::available_capacity() );

	// Split the data into two segments if it exceeds the available capacity
	uint64_t first_segment_size = std::min( size_of_write, capacity_ - tail_ );
	std::memcpy( buffer_.data() + tail_, data.data(), first_segment_size );

	// If the data exceeds the available capacity, copy the remaining data to the beginning of the buffer
	if ( size_of_write > first_segment_size ) {
		uint64_t second_segment_size = size_of_write - first_segment_size;
		std::memcpy( buffer_.data(), data.data() + first_segment_size, second_segment_size );
	}

	tail_ = ( tail_ + size_of_write ) % capacity_;
	size_ += size_of_write;
	bytes_pushed_ += size_of_write;
}

void Writer::close()
{
	is_closed_ = true;
}

bool Writer::is_closed() const
{
	return is_closed_;
}

uint64_t Writer::available_capacity() const
{
	return capacity_ - size_;
}

uint64_t Writer::bytes_pushed() const
{
	return bytes_pushed_;
}

string_view Reader::peek() const
{
	if ( size_ == 0 ) {
		return string_view();
	}

	uint64_t peek_size = std::min( size_, capacity_ - head_ );
	return string_view( buffer_.data() + head_, peek_size );
}

void Reader::pop( uint64_t len )
{
	uint64_t pop_len = std::min( len, size_ );

	head_ = ( head_ + pop_len ) % capacity_;
	size_ -= pop_len;
	bytes_popped_ += pop_len;
}

bool Reader::is_finished() const
{
	return is_closed_ && size_ == 0;
}

uint64_t Reader::bytes_buffered() const
{
	return size_;
}

uint64_t Reader::bytes_popped() const
{
	return bytes_popped_;
}
