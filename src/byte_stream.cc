#include "byte_stream.hh"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <sys/types.h>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_( capacity ) {}

/**
 * @brief Push data into the stream.
 *
 * This function writes the provided data into the ByteStream. If the data size
 * exceeds the available capacity, it will split the data into two parts:
 * - One part will be written starting from the current `tail_` position.
 * - The other part will be written at the beginning of the buffer if necessary.
 *
 * The `tail_` pointer is updated after the data is written.
 *
 * @param data The data to be written into the ByteStream.
 */
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

/**
 * @brief Peek at the data in the Reader without removing it.
 *
 * This function allows the user to view (but not modify) the data in the
 * stream starting from the `head_` position. The size of the peeked data
 * is limited by either the available data size or the remaining capacity in the buffer.
 *
 * @return A string_view of the data starting from the `head_` position.
 */
string_view Reader::peek() const
{
	if ( size_ == 0 ) {
		return string_view();
	}

	uint64_t peek_size = std::min( size_, capacity_ - head_ );
	return string_view( buffer_.data() + head_, peek_size );
}

/**
 * @brief Pop (remove) data from the Reader.
 *
 * This function removes up to `len` bytes from the stream starting from
 * the current `head_` position. The `head_` pointer is updated accordingly,
 * and the size of the stream is reduced.
 *
 * @param len The number of bytes to be removed from the stream.
 */
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
