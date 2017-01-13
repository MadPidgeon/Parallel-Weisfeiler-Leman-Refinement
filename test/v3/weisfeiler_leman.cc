/*
g++ -o WL weisfeiler_leman.cc colour.o city.o lib/libmcbsp1.2.0.a -pthread -std=c++14
*/
#include <iostream>
#include <set>
#include <vector>
#include <array>
#include <cmath>
#include <map>
#include <algorithm>
#include <sstream>
#include <string>
#include <cstring>
#include <unordered_set>
#include "colour.h"
#include "../ext.h"
#include "../include/mcbsp.hpp"
#include "city.h"

extern "C" {
	#include "../mcbsp-affinity.h"
}

using namespace std;

int P; // number of processors
int N; // number of nodes in graph
int T; // number of tuples of nodes in graph

uint m_index( uint a, uint b ) {
	return a+b*N;
}

void matrix_broadcast( int p, vector<colour_t>& matrix ) {
	int B = (T+P-1)/P;
	if( p == 0 )
		for( int i = 1; i < P; ++i )
			bsp_put( i, matrix.data()+B*i, matrix.data(), B*i*sizeof(colour_t), min( B, T-B*i )*sizeof(colour_t) );
	bsp_sync();
	for( int i = 1; i < P; ++i )
		if( i != p )
			bsp_put( i, matrix.data()+B*p, matrix.data(), B*p*sizeof(colour_t), min( B, T-B*p )*sizeof(colour_t) );	
}

void input_matrix( int p, vector<colour_t>& M ) {
	if( p == 0 )
		for( int i = 0; i < T; ++i )
			M[i].assign( getchar()+1 );
	for( int i = 0; i < N; ++i )
		M[m_index(i,i)].assign( 0 );
	matrix_broadcast( p, M );
	bsp_sync();
}

void output_matrix( int p, const vector<colour_t>& matrix ) {
	if( p == 0 ) {
		ostream& os = cout;
		os << "[ ";
		for( int j = 0; j < N; ++j ) {
			os << "[ ";
			for( int i = 0; i < N; ++i )
				os << matrix[i+j*N] << " ";
			os << "] ";
		}
		os << "]";
	}
}

vector<int> clean_matrix( const vector<colour_t>& matrix ) { // not canonical
	size_t nn = matrix.size();
	map<colour_t,int> M;
	vector<int> R( nn );
	int c = 0;
	for( int i = 0; i < nn; ++i ) {
		auto f = M.find( matrix[i] );
		if( f == M.end() ) {
			R[i] = c;
			M[matrix[i]] = c++; 
		} else
			R[i] = f->second;
	}
	return R;
}

vector<vertex_tuple_t> edge_distribution( int p ) {
	// ugly, but easiest to change distribution with
	vector<vertex_tuple_t> local_edges;
	uint start = p*(T/P), end = (p+1)*(T/P);
	if( p == P-1 )
		end = T;
	local_edges.reserve( T/P );
	for( uint i = start; i < end; ++i )
		local_edges.push_back({ i / N, i % N });
	return local_edges;
}

vector<vertex_tuple_t> block_edge_distribution( int p ) {
	int ks = N / (2*P);
	int rs = 2*ks;
	int c = 0;
	vector<vertex_tuple_t> L;
	for( int x = p*rs; x < (p+1)*rs; ++x )
		for( int y = x; y >= p*rs; --y )
			L.push_back( {y,x} );
	for( int x = (p+1)*rs; x < N; ++x )
		for( int y = p*rs; y < p*rs+ks; ++y )
			L.push_back( {y,x} );
	for( int it = 0; it < p; ++it )
		for( int x = p*rs; x < (p+1)*rs; ++x  )
			for( int y = it*rs+ks; y < (it+1)*rs; ++y )
				L.push_back( {y,x} );
	for( int x = P*rs; x < N; ++x ) 
		for( int y = p*rs; y < (p+1)*rs; ++y ) 
			L.push_back( {y,x} );
	for( int x = P*rs; x < N; ++x )
		for( int y = P*rs; y <= x; ++y )
			if( (c++) % P == p )
				L.push_back( {y,x} );
	return L;
}

void processor_main() {
	bsp_begin( P );
	unsigned int p = bsp_pid();
	uint iterations = 0;
	uint relation_count = 3;
	bool parity = false;
	vector<colour_t> coloured_graph[2] = { vector<colour_t>( T ), vector<colour_t>( T ) };
	bsp_push_reg( coloured_graph[0].data(), sizeof(colour_t)*T );
	bsp_push_reg( coloured_graph[1].data(), sizeof(colour_t)*T );
	auto local_edges = block_edge_distribution( p );
	unordered_set<colour_t> colours;

	bsp_sync();
	input_matrix( p, coloured_graph[parity] );
	
	bool unstable = true;
	while( unstable ) {
		auto& M = coloured_graph[parity];
		auto& newM = coloured_graph[not parity];
		unstable = false;

		// iterator over the local edges
		for( auto t : local_edges ) {
			colour_pattern_t colour_pattern;
			uint index = m_index( t[0], t[1] );
			colour_t old_colour = M[index];
			// generate new colour for the edge through the colour pattern
			for( int i = 0; i < N; ++i )
				colour_pattern.push_back({ M[m_index(t[0],i)], M[m_index(i,t[1])] });
			newM[index].assign( old_colour, colour_pattern );
			// share new edge colour
			for( int q = 0; q < P; ++q )
				if( q != p )
					bsp_put( q, newM.data()+index, newM.data(), index*sizeof( colour_t ), sizeof( colour_t ) );
		}

		parity = not parity;
		iterations++;
		bsp_sync();

		for( int y = 0; y < N; ++y )
			for( int x = y+1; x < N; ++x )
				newM[m_index( x, y )] = newM[m_index( y, x )].reverse();

		for( int i = 0; i < T; ++i )
			colours.insert( coloured_graph[parity][i] );
		if( colours.size() != relation_count )
			unstable = true;
		relation_count = colours.size();
		colours.clear();
	}

	/*if( p == 0 )
		cout << clean_matrix( coloured_graph[parity] ) << endl;*/
	//output_matrix( p, coloured_graph[parity] );
	if( p == 0 )
		cout << endl << "iterations: " << iterations << endl;
	bsp_end();
}

int main( int argc, char ** argv ) {
	bsp_init( processor_main, argc, argv );
	mcbsp_set_affinity_mode( COMPACT );
	if( argc != 2 ) {
		cout << "Missing processor count!" << endl;
		return 1;
	}
	P = atoi( argv[1] );
	scanf( "%d %*[ \n\t]", &N );
	// usage: processors nodes matrix
	T = N*N;
	processor_main();
	return 0;
}