/*
g++ -c colour.cc -std=c++11 
*/

#include "colour.h"
#include "city.h"

using namespace std;

size_t hash_t::raw( char* s ) const {
	memcpy( s, &value, sizeof( colour_t ) );
	return sizeof( colour_t );
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
	return os << std::hex << H.value.first << H.value.second << std::dec;
}

/*std::istream& operator>>( std::istream& is, hash_t& H ) {

}*/

void colour_t::assign( colour_t col, bool sym, colour_pattern_t pat ) {
	original_colour = col.original_colour;
	is_symmetrical = sym;
	size_t l = sizeof( colour_t ), p = pat.size(), n = (1+2*p)*l;
	char* s = new char[ n ];
	col.hash_value.raw( s );
	for( int i = 0; i < p; ++i )
		for( int j = 0; j < 2; ++j )
			pat[i][j].hash_value.raw( s + (1 + i*2 + j)*l );
	hash_value.assign( s, n );
	delete [] s;
}

colour_t::colour_t( colour_t col, bool sym, colour_pattern_t pat ) {
	assign( col, sym, pat );
}


void colour_t::assign( uint64_t v ) {
	original_colour = v;
	is_symmetrical = 1;
	hash_value.assign( v );
}

colour_t::colour_t( uint64_t v ) {
	assign( v );
}

colour_t::colour_t() {
	assign( 1337 );
}

bool colour_t::operator==( const colour_t& other ) const {
	return hash_value == other.hash_value and
		original_colour == other.original_colour and 
		is_symmetrical == other.is_symmetrical;
}

bool colour_t::operator<( const colour_t& other ) const {
	if( hash_value < other.hash_value )
		return true;
	else if( other.hash_value < hash_value )
		return false;
	if( original_colour < other.original_colour )
		return true;
	else if( original_colour > other.original_colour )
		return false;
	if( ( not is_symmetrical ) and other.is_symmetrical )
		return true;
	return false;
}

std::ostream& operator<<( std::ostream& os, colour_t C ) {
	return os << C.hash_value;
}