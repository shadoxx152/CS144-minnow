#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <sys/types.h>
#include <utility>

using namespace std;

/**
 * @brief Insert a new substring to be reassembled into a ByteStream.
 * There are five cases to handle:
 * 1. The stream has been closed.
 * 2. The data to be written has been overwritten by the data already written.
 * 3. The data that has been written overlaps with the data to be inserted.
 * 4. Some data exceeds capacity.
 * 5. The data before the substring has not been completely written.
 *
 * Among them, cases 2, 3, and 4 are in function handle_substring.
 * The fifth case is handled in the merge_into_buffered_data function, this function complete the merge operation
 * when data inserted into the buffered_data_.
 *
 * Finally, call write_to_output to write the data to the output stream and address the buffered data.
 *
 * @param first_index The index of the first byte of the substring
 * @param data The substring itself
 * @param is_last_substring This substring represents the end of the stream
 */
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
	/// first case: The stream has been closed
	if ( output_.writer().is_closed() ) {
		return;
	}

	/// second, third, fourth cases: The data need to be handled, detailed in handle_substring
	handle_substring( first_index, data, is_last_substring );

	uint64_t bytes_pushed = output_.writer().bytes_pushed();

	/// fifth case: The data before the substring has not been completely written
	if ( first_index > bytes_pushed ) {
		merge_into_buffered_data( first_index, data, is_last_substring );
		return; /// Data not yet written
	}

	write_to_output( first_index, std::move( data ), is_last_substring );
}

/**
 * @brief Count the number of bytes pending in the Reassembler.
 */
uint64_t Reassembler::count_bytes_pending() const
{
	uint64_t count = 0;

	for ( const auto& entry : buffered_data_ ) {
		count += entry.second.first.size();
	}

	return count;
}

/**
 * @brief Write the data to the output stream and handle buffered data.
 *
 * At first, write the data to the output stream.
 * Then, check the buffered data and write it to the output stream if possible.
 *
 * For buffer data write operations, there are the following situations:
 * 1. The data before it not completely wriiten, break the loop because map is ordered.
 * 2. The data before it has been completely written and no overlapping, write the data to the output stream and
 * erase it from the buffered_data_.
 * 3. The data before it has been completely written and overlapping, write the data to the output stream and erase
 * it from the buffered_data_. In the third case, there are two another situations: 3.1 The data has been
 * overridden, write the data to the output stream and erase it from the buffered_data_. 3.2 The data has not been
 * overridden, write the data to the output stream and erase it from the buffered_data_.
 *
 * @param first_index The index of the first byte of the substring
 * @param data The substring itself
 * @param is_last_substring This substring represents the end of the stream
 */
void Reassembler::write_to_output( uint64_t first_index, string&& data, bool is_last_substring )
{
	static int debug_count = 0;
	if ( debug_count++ < 10 ) {
		debug( "write_to_output({}, size={}, {})", first_index, data.size(), is_last_substring );
	}

	output_.writer().push( std::move( data ) );

	if ( is_last_substring ) {
		output_.writer().close();
	}

	while ( !buffered_data_.empty() ) {
		auto it = buffered_data_.begin();
		uint64_t it_first_index = it->first;
		uint64_t it_last_index = it_first_index + it->second.first.size() - 1;

		if ( it_first_index > output_.writer().bytes_pushed() ) {
			break; /// No more data to write
		}

		if ( it_first_index == output_.writer().bytes_pushed() ) {
			output_.writer().push( std::move( it->second.first ) );
			if ( it->second.second ) {
				output_.writer().close();
			}
			buffered_data_.erase( it );
			continue; /// Data has been written
		}

		if ( it_last_index < output_.writer().bytes_pushed() ) {
			buffered_data_.erase( it );
			continue; /// Data has already been written
		}

		std::string new_data = it->second.first.substr( output_.writer().bytes_pushed() - it_first_index );

		output_.writer().push( std::move( new_data ) );
		if ( it->second.second ) {
			output_.writer().close();
		}
		buffered_data_.erase( it );
	}
}

/**
 * @brief Handle the substring to be inserted into the output stream.
 * This function handles the case 2, 3, and 4 of the insert function.
 * @param first_index The index of the first byte of the substring
 * @param data The substring itself
 * @param is_last_substring This substring represents the end of the stream
 */
void Reassembler::handle_substring( uint64_t& first_index, string& data, bool& is_last_substring )
{
	uint64_t bytes_pushed = output_.writer().bytes_pushed();
	uint64_t available_capacity = output_.writer().available_capacity();

	// second case: The data to be written has been overwritten by the data already written
	if ( bytes_pushed >= first_index + data.size() ) {
		data.clear(); // Data has already been written, so clear the string.
		return;
	}

	/// third case: The data that has been written overlaps with the data to be inserted
	if ( first_index < bytes_pushed ) {
		size_t offset = bytes_pushed - first_index;
		if ( offset >= data.size() ) {
			data.clear();
			return;
		}
		data = data.substr( offset );
		first_index = bytes_pushed;
	}

	/// fourth case: Some data exceeds capacity
	if ( first_index + data.size() > available_capacity + bytes_pushed ) {
		data = data.substr( 0,
							std::max( static_cast<size_t>( 0 ),
									  available_capacity + bytes_pushed
										- first_index ) ); /// Trim data to fit within the available capacity.

		// If this substring is the last one, we keep the part that can be accommodated, and this part is not the
		// last one.
		is_last_substring = false;
	}
}

/**
 * @brief Merge the new substring into the buffered data.
 * First, find the lower bound of the first index in the buffered data, and check if the previous data overlaps with
 * the new substring, if so, merge them. Then merge the new substring with later buffered data, if there is any
 * overlap, merge and erase the buffered data. Finally, insert the merged data into the buffered_data_.
 * @param first_index The index of the first byte of the substring
 * @param data The substring itself
 * @param is_last_substring This substring represents the end of the stream
 */
void Reassembler::merge_into_buffered_data( uint64_t first_index, std::string data, bool is_last_substring )
{
	uint64_t current_last_index = first_index + data.size() - 1;
	uint64_t current_first_index = first_index;
	auto it = buffered_data_.lower_bound( first_index );

	if ( it != buffered_data_.begin() ) {
		auto prev_it = std::prev( it );
		uint64_t prev_end = prev_it->first + prev_it->second.first.size() - 1;

		if ( prev_end >= first_index - 1 ) {
			first_index = prev_it->first;

			if ( current_last_index <= prev_end ) {
				return; /// The new data is completely covered by the previous data
			}

			std::string merged_data;

			merged_data = prev_it->second.first.substr( 0, current_first_index - prev_it->first ) + data;

			/// Update loop variables
			data = std::move( merged_data );
			current_first_index = prev_it->first;
			// current_last_index not changed
			is_last_substring |= prev_it->second.second;

			buffered_data_.erase( prev_it );
		}
	}

	while ( ( it = buffered_data_.lower_bound( current_first_index ) ) != buffered_data_.end()
			&& it->first <= current_last_index ) {
		uint64_t it_first_index = it->first;
		uint64_t it_last_index = it_first_index + it->second.first.size() - 1;

		if ( current_last_index >= it_last_index ) {
			/// current_last_index not changed
			is_last_substring |= it->second.second;
			buffered_data_.erase( it );
			continue;
		}

		data = data.substr( 0, it_first_index - current_first_index ) + it->second.first;
		/// current_first_index not changed
		current_last_index = it_last_index;
		is_last_substring |= it->second.second;
		buffered_data_.erase( it );
	}

	buffered_data_[current_first_index] = std::make_pair( std::move( data ), is_last_substring );
}
