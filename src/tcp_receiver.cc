#include "tcp_receiver.hh"
#include "tcp_receiver_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <sys/types.h>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
	if ( message.RST ) {
		reassembler_.set_error();
		return;
	}

	if ( message.SYN && !ISN_.has_value() ) {
		ISN_ = message.seqno;
		FIN_ = false;
	}

	if ( message.FIN ) {
		FIN_ = true;
	}

	if ( !ISN_.has_value() ) {
		return;
	}

	/*debug( "reassembler_.writer().bytes_pushed()={}\n,reassembler_.get_FIN={}\n,reassembler_.get_SYN={}\n,
	 * ISN_={}",*/
	/*	   reassembler_.writer().bytes_pushed(),*/
	/*	   reassembler_.get_FIN(),*/
	/*	   reassembler_.get_SYN(),*/
	/*	   ISN_->raw_value() );*/
	/*uint64_t first_index = ISN_->unwrap(*/
	/*  *ISN_, reassembler_.writer().bytes_pushed() + reassembler_.get_FIN() + reassembler_.get_SYN() );*/
	/**/
	/*reassembler_.insert( first_index, message.payload, message.FIN );*/
	uint64_t abs_seqno = message.seqno.unwrap( *ISN_, checkpoint_ );

	uint64_t stream_index;

	if ( message.SYN ) {
		stream_index = 0;
	} else {
		stream_index = abs_seqno - 1;
	}

	reassembler_.insert( stream_index, message.payload, message.FIN );
	checkpoint_ = abs_seqno + message.payload.size();
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

	message.window_size = std::min( reassembler_.writer().available_capacity(),
									static_cast<uint64_t>( std::numeric_limits<uint16_t>::max() ) );
	return message;
}
