#include "tcp_receiver.hh"
#include "debug.hh"
#include "tcp_receiver_message.hh"
#include "wrapping_integers.hh"
#include <cstdint>
#include <sys/types.h>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
	/*if ( message.RST ) {*/
	/*	reassembler_.output_.set_error();*/
	/*	return;*/
	/*}*/

	if ( message.SYN && !ISN_.has_value() ) {
		ISN_ = message.seqno;
	}

	if ( !ISN_.has_value() ) {
		return;
	}

	uint64_t unwraped_seqno = ISN_->unwrap( *ISN_, checkpoint_ );

	uint64_t stream_index = unwraped_seqno + ( message.SYN ? 1 : 0 );

	reassembler_.insert( stream_index, message.payload, message.FIN );

	if ( message.sequence_length() > 0 ) {
		checkpoint_ = unwraped_seqno + message.sequence_length() - 1;
	}

	if ( message.FIN ) {
		FIN_ = true;
	}
}

TCPReceiverMessage TCPReceiver::send() const
{
	TCPReceiverMessage message;

	if ( reassembler_.writer().has_error() ) {
		message.RST = true;
	}

	if ( ISN_.has_value() ) {
		uint64_t ackno = reassembler_.writer().bytes_pushed() + 1;
		if ( FIN_ && reassembler_.writer().is_closed() ) {
			ackno += 1;
		}
		message.ackno = Wrap32::wrap( ackno, *ISN_ );
	}

	message.window_size = reassembler_.writer().available_capacity();
	return message;
}
