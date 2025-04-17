#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <spanstream>

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
	/*debug( "unimplemented sequence_numbers_in_flight() called" );*/
	/*return {};*/

	uint64_t in_flight = 0;
	for ( const auto& entry : outstanding_ ) {
		in_flight += entry.second.sequence_length();
	}

	return in_flight;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
	/*debug( "unimplemented consecutive_retransmissions() called" );*/
	/*return {};*/
	return retran_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
	/*debug( "unimplemented push() called" );*/
	if ( !SYN_ ) {
		TCPSenderMessage syn_msg;
		syn_msg.SYN = true;
		syn_msg.seqno = Wrap32::wrap( last_seq_, isn_ );

		transmit( syn_msg );

		outstanding_[last_seq_] = syn_msg;
		last_seq_ += 1;
		SYN_ = true;

		if ( !tcp_sender_timer_.is_running() ) {
			tcp_sender_timer_.start();
		}
		return;
	}

	while ( last_seq_ < sender_window_right_edge() ) {
		TCPSenderMessage msg;
		msg.seqno = Wrap32::wrap( last_seq_, isn_ );

		uint64_t space = std::min( sender_window_size(), TCPConfig::MAX_PAYLOAD_SIZE );

		msg.payload = reader().peek().substr( 0, space );

		bool is_fin
		  = writer().is_closed() && !FIN_ && ( last_seq_ + msg.sequence_length() < sender_window_right_edge() );

		if ( is_fin ) {
			msg.FIN = true;
			FIN_ = true;
		}

		if ( msg.sequence_length() == 0 ) {
			break;
		}

		transmit( msg );

		outstanding_[last_seq_] = msg;
		last_seq_ += msg.sequence_length();

		if ( !tcp_sender_timer_.is_running() ) {
			tcp_sender_timer_.start();
		}
	}
}

TCPSenderMessage TCPSender::make_empty_message() const
{
	/*debug( "unimplemented make_empty_message() called" );*/
	/*return {};*/

	TCPSenderMessage msg;

	msg.seqno = Wrap32::wrap( last_seq_, isn_ );

	return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
	if ( msg.RST ) {
		input_.set_error();
	}

	if ( !msg.ackno.has_value() ) {
		return;
	}

	uint64_t unwrap_ackno = msg.ackno->unwrap( isn_, last_ack_ );
	receiver_window_size_ = msg.window_size;

	if ( unwrap_ackno > last_seq_ ) {
		return;
	}

	if ( unwrap_ackno <= last_ack_ ) {
		return;
	}

	last_ack_ = unwrap_ackno;

	auto it = outstanding_.begin();
	while ( it != outstanding_.end() ) {
		if ( it->first + it->second.sequence_length() <= last_ack_ ) {
			it = outstanding_.erase( it );
			continue;
		}

		++it;
	}

	tcp_sender_timer_.reset();
	retran_cnt_ = 0;

	if ( !outstanding_.empty() ) {
		tcp_sender_timer_.start();
		return;
	}

	tcp_sender_timer_.stop();
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
