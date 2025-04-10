#include "wrapping_integers.hh"
#include "debug.hh"
#include <cstdint>

using namespace std;

/**
 *
 */
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
	return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
	const uint32_t offset = raw_value_ - zero_point.raw_value_;
	const uint64_t base = checkpoint & 0xFFFFFFFF00000000;

	uint64_t candidate = base + offset;

	if ( candidate + ( 1ul << 31 ) < checkpoint ) {
		candidate += ( 1ul << 32 );
	} else if ( candidate > checkpoint + ( 1ul << 31 ) ) {
		candidate -= ( 1ul << 32 );
	}

	return candidate;
}
