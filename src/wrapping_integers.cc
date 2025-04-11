#include "wrapping_integers.hh"
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
	const uint64_t base = checkpoint & 0xFFFFFFFF00000000;
	uint64_t candidate = base + ( raw_value_ - zero_point.raw_value_ );

	const uint64_t alt1 = candidate + ( 1ull << 32 );
	const uint64_t alt2 = candidate - ( 1ull << 32 );

	if ( candidate >= checkpoint ) {
		if ( candidate - checkpoint > ( 1ull << 31 ) && alt2 <= checkpoint ) {
			return alt2;
		}
	} else {
		if ( checkpoint - candidate > ( 1ull << 31 ) ) {
			return alt1;
		}
	}

	return candidate;
}
