#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : buffer( capacity )
  , // Default initialize the vector (empty buffer)
  head( 0 )
  , // Initialize head to 0
  tail( 0 )
  , // Initialize tail to 0
  buffer_size( 0 )
  , // Initialize buffer_size to 0
  bytes_written( 0 )
  , // Initialize bytes_written to 0
  bytes_read( 0 )
  , // Initialize bytes_read to 0
  capacity_( capacity )
  , // Initialize capacity_ with the value of the constructor argument
  error_( false )
  ,				   // Initialize error_ to false
  closed_( false ) // Initialize closed_ to false
{}

void Writer::push( string data )
{
	// Check if the rest of the data can fit in the buffer
	if ( data.size() >= Writer::available_capacity() ) {
		data.resize( Writer::available_capacity() );
	}

	// Push the data to the buffer
	for ( size_t i = 0; i < data.size(); ++i ) {
		buffer[tail] = data[i];
		tail = ( tail + 1 ) % capacity_;
	}

	// Update the size of the buffer
	buffer_size += data.size();
	// Update the number of bytes written
	bytes_written += data.size();
}

void Writer::close()
{
	closed_ = true;
}

bool Writer::is_closed() const
{
	return closed_;
}

uint64_t Writer::available_capacity() const
{
	return capacity_ - buffer_size;
}

uint64_t Writer::bytes_pushed() const
{
	return bytes_written;
}

string_view Reader::peek() const
{
	// Check if the buffer is empty
	if ( buffer_size == 0 ) {
		return string_view();
	}

	// Create a string_view to the data in the buffer
	const uint64_t contiguous_length = min( buffer_size, capacity_ - head );
	return string_view( buffer.data() + head, contiguous_length );
}

void Reader::pop( uint64_t len )
{
	if ( len > buffer_size ) {
		len = buffer_size;
	}

	// Pop the data from the buffer
	head = ( head + len ) % capacity_;
	buffer_size -= len;
	bytes_read += len;
}

bool Reader::is_finished() const
{
	return closed_ && ( buffer_size == 0 );
}

uint64_t Reader::bytes_buffered() const
{
	return buffer_size;
}

uint64_t Reader::bytes_popped() const
{
	return bytes_read;
}
