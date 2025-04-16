#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <ctime>
#include <functional>
#include <map>

class TCPSenderTimer
{
  public:
	explicit TCPSenderTimer( uint64_t inital_RTO_ms ) : inital_RTO_ms_( inital_RTO_ms ) {}

	void start()
	{
		is_running_ = true;
		accumulation_time_ = 0;
	}

	void stop() { is_running_ = false; };

	void reset()
	{
		current_RTO_ms_ = inital_RTO_ms_;
		accumulation_time_ = 0;
	}

	void double_RTO() { current_RTO_ms_ *= 2; }

	void time_acumulatetive( uint64_t ms_since_last_tick ) { accumulation_time_ += ms_since_last_tick; }

	bool is_expire() { return !is_running_ && accumulation_time_ >= current_RTO_ms_; }

	bool is_running() { return is_running_; }

  private:
	uint64_t inital_RTO_ms_;
	uint64_t current_RTO_ms_;
	uint64_t accumulation_time_ {};

	bool is_running_ {};
};

class TCPSender
{
  public:
	/* Construct TCP sender with given default Retransmission Timeout and possible ISN */
	TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
	  : input_( std::move( input ) )
	  , isn_( isn )
	  , initial_RTO_ms_( initial_RTO_ms )
	  , tcp_sender_timer_( initial_RTO_ms )
	{}

	/* Generate an empty TCPSenderMessage */
	TCPSenderMessage make_empty_message() const;

	/* Receive and process a TCPReceiverMessage from the peer's receiver */
	void receive( const TCPReceiverMessage& msg );

	/* Type of the `transmit` function that the push and tick methods can use to send messages */
	using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

	/* Push bytes from the outbound stream */
	void push( const TransmitFunction& transmit );

	/* Time has passed by the given # of milliseconds since the last time the tick() method was called */
	void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

	// Accessors
	uint64_t sequence_numbers_in_flight() const; // For testing: how many sequence numbers are outstanding?
	uint64_t consecutive_retransmissions()
	  const; // For testing: how many consecutive retransmissions have happened?
	const Writer& writer() const { return input_.writer(); }
	const Reader& reader() const { return input_.reader(); }
	Writer& writer() { return input_.writer(); }

  private:
	Reader& reader() { return input_.reader(); }

	ByteStream input_;
	Wrap32 isn_;
	uint64_t initial_RTO_ms_;

	std::map<uint64_t, TCPSenderMessage> outstanding_ {};

	TCPSenderTimer tcp_sender_timer_;
	uint64_t retran_cnt_ {};

	uint16_t sender_window_size_ = 1;
	uint16_t receiver_window_size_ = 1;

	uint64_t last_ack_ {};
	uint64_t last_seq_ {};
};
