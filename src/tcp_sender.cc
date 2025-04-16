#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include <cstdint>

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
	debug( "unimplemented sequence_numbers_in_flight() called" );
	return {};
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
	debug( "unimplemented consecutive_retransmissions() called" );
	return {};
}

void TCPSender::push( const TransmitFunction& transmit )
{
	debug( "unimplemented push() called" );
	(void)transmit;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
	debug( "unimplemented make_empty_message() called" );
	return {};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
	if ( msg.RST ) {
		input_.set_error();
	}

	uint64_t unwrap_ackno = msg.ackno->unwrap( isn_, last_ack_ );
	receiver_window_size_ = msg.window_size;

	if ( unwrap_ackno > last_seq_ ) {
		return;
	}

	if ( unwrap_ackno )
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
	/*debug( "unimplemented tick({}, ...) called", ms_since_last_tick );*/
	/*(void)transmit;*/

	if ( !tcp_sender_timer_.is_running() ) {
		return;
	}

	tcp_sender_timer_.time_acumulatetive( ms_since_last_tick );

	if ( tcp_sender_timer_.is_expire() && !outstanding_.empty() ) {
		transmit( outstanding_.begin()->second );

		if ( receiver_window_size_ != 0 ) {
			++retran_cnt_;
			tcp_sender_timer_.double_RTO();
		}
		tcp_sender_timer_.start();
	}
}
