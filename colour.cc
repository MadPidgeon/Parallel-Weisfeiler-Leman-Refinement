/*
g++ -c colour.cc -std=c++11 
*/

#include "colour.h"
#include "city.h"

using namespace std;

size_t hash_t::raw( char* s ) const {
	memcpy( s, &value, sizeof( hash_t ) );
	return sizeof( hash_t );
}

void hash_t::assign( const char* str, size_t len ) {
	value = CityHash128( str, len );
}

void hash_t::assign( uint64_t v ) {
	value.first = 0;
	value.second = v;
}

bool hash_t::operator==( const hash_t& other ) const {
	return value == other.value;
}

bool hash_t::operator<( const hash_t& other ) const {
	return value < other.value;
}

hash_t::hash_t() : hash_t( 0 ) {
}

hash_t::hash_t( uint64_t v ) {
	assign( v );
}

hash_t::hash_t( const char* str, size_t len ) {
	assign( str, len );
}

std::ostream& operator<<( std::ostream& os, hash_t H ) {
	return os << std::hex << std::setfill('0') << std::setw(16) << H.value.first << std::setw(16) << H.value.second << std::dec;
}

struct colour_pattern_sort_functor_t {
	const int i;
	colour_pattern_sort_functor_t( int _i ) : i(_i) {} 
	bool operator()( const std::array<colour_t,k>& a, const std::array<colour_t,k>& b ) const { // assumes k = 2
		// for i = 0: sorts regularly on first component, then second component
		// for i = 1: inversion sort on second component, then first component
		return a[i].less_than( b[i], i ) or ( a[i] == b[i] and a[!i].less_than( b[!i], i ) );
	}
};

void hex_out( ostream& ss, char* s, int l ) {
	ss << std::hex;
	for( int i = l-1; i >= 0; --i )
		ss << std::setw(2) << int( s[i] );
	ss << std::dec;
}

void colour_t::assign( colour_t col, colour_pattern_t pat ) {
	#pragma message("Warning: Your code assumes k = 2!")
	original_colour = col.original_colour;
	size_t l = sizeof( hash_t ), p = pat.size(), n = (1+k*p)*l;
	char* s = new char[ n ];
	for( int h = 0; h < 2; ++h ) { // for some reason this fails
		std::sort( pat.begin(), pat.end(), colour_pattern_sort_functor_t(h) );
		col.hash_value[h].raw( s );
		for( int i = 0; i < p; ++i )
			for( int j = 0; j < k; ++j ) // assumes k = 2 
				pat[i][j^h].hash_value[h].raw( s + (1 + i*k + j)*l );
		hash_value[h].assign( s, n );
	}
	delete [] s;
}

colour_t::colour_t( colour_t col, colour_pattern_t pat ) {
	assign( col, pat );
}


void colour_t::assign( uint64_t v ) {
	original_colour = v;
	hash_value[0].assign( v );
	hash_value[1] = hash_value[0];
}

colour_t colour_t::reverse() const {
	colour_t r( original_colour );
	r.hash_value[0] = hash_value[1];
	r.hash_value[1] = hash_value[0];
	return r;
}

colour_t::colour_t( uint64_t v ) {
	assign( v );
}

colour_t::colour_t() {
	assign( 524421 ); // arbitrary constant, recognisable for debugging purposes
}

bool colour_t::operator==( const colour_t& other ) const {
	return hash_value[0] == other.hash_value[0] and
		hash_value[1] == other.hash_value[1] and
		original_colour == other.original_colour;
}

bool colour_t::less_than( const colour_t& other, int i ) const {
	if( hash_value[i] < other.hash_value[i] )
		return true;
	else if( other.hash_value[i] < hash_value[i] )
		return false;
	if( hash_value[!i] < other.hash_value[!i] )
		return true;
	else if( other.hash_value[!i] < hash_value[!i] )
		return false;
	if( original_colour < other.original_colour )
		return true;
	else if( original_colour > other.original_colour )
		return false;
	return false;
}

bool colour_t::operator<( const colour_t& other ) const { // non-canonical
	return less_than( other, 0 );
}

std::ostream& operator<<( std::ostream& os, colour_t C ) {
	return os << "(" << C.hash_value[0] << ":" << C.hash_value[1] << ")";
}