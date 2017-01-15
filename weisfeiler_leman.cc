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
#include "include/mcbsp.hpp"
#include "city.h"

extern "C" {
	#include "mcbsp-affinity.h"
}

using namespace std;

int P; // number of processors
int N; // number of nodes in graph
int T; // number of tuples of nodes in graph ( = N^2 )

// converts a matrix index to an array index
uint m_index( uint a, uint b ) {
	return a+b*N;
}

// sends a matrix from processor 0 to all other processors
// assumes matrix is already pushed to the bsp register
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

// scan matrix from standard input into processor 0
void input_matrix( int p, vector<colour_t>& M ) {
	if( p == 0 )
		for( int i = 0; i < T; ++i )
			M[i].assign( getchar()+1 );
	for( int i = 0; i < N; ++i )
		M[m_index(i,i)].assign( 0 );
	matrix_broadcast( p, M );
	bsp_sync();
}

// print the matrix to standard output from processor 0
void output_matrix( int p, const vector<colour_t>& matrix ) {
	if( p == 0 ) {
		cout << "[ ";
		for( int j = 0; j < N; ++j ) {
			cout << "[ ";
			for( int i = 0; i < N; ++i )
				cout << matrix[i+j*N] << " ";
			cout << "] ";
		}
		cout << "]";
	}
}

// returns a more readable matrix representation than output_matrix
// WARNING: this representation is NOT canonical
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

// defines a simple distribution of the tuples amongst the processors
vector<vertex_tuple_t> edge_distribution( int p ) {
	vector<vertex_tuple_t> local_edges;
	uint start = p*(T/P), end = (p+1)*(T/P);
	if( p == P-1 )
		end = T;
	local_edges.reserve( T/P );
	for( uint i = start; i < end; ++i )
		local_edges.push_back({ i / N, i % N });
	return local_edges;
}

// defines a upper-diagonal block distribution of tuples amongst the processors
vector<vertex_tuple_t> block_edge_distribution( int p ) {
	int ks = N / (2*P);
	int rs = 2*ks;
	int c = 0;
	vector<vertex_tuple_t> L;
	for( int x = p*rs; x < (p+1)*rs; ++x )
		for( int y = x; y >= p*rs; --y )
			L.push_back( {uint(y),uint(x)} );
	for( int x = (p+1)*rs; x < N; ++x )
		for( int y = p*rs; y < p*rs+ks; ++y )
			L.push_back( {uint(y),uint(x)} );
	for( int it = 0; it < p; ++it )
		for( int x = p*rs; x < (p+1)*rs; ++x  )
			for( int y = it*rs+ks; y < (it+1)*rs; ++y )
				L.push_back( {uint(y),uint(x)} );
	for( int x = P*rs; x < N; ++x ) 
		for( int y = p*rs; y < (p+1)*rs; ++y ) 
			L.push_back( {uint(y),uint(x)} );
	for( int x = P*rs; x < N; ++x )
		for( int y = P*rs; y <= x; ++y )
			if( (c++) % P == p )
				L.push_back( {uint(y),uint(x)} );
	return L;
}

// main processor function
void processor_main() {
	bsp_begin( P );
	unsigned int p = bsp_pid(); // processor id
	uint iterations = 0; // counts the number of iterations
	uint relation_count = 3; // counts the number of starting relations for stability check
	bool parity = false; // (= iterations % 2)
	auto local_edges = block_edge_distribution( p ); // pairs assigned to this processor
	unordered_set<colour_t> colours; // collection of all colours in configuration after iteration

	// register input and output matrices to BSP library
	vector<colour_t> coloured_graph[2] = { vector<colour_t>( T ), vector<colour_t>( T ) };
	bsp_push_reg( coloured_graph[0].data(), sizeof(colour_t)*T );
	bsp_push_reg( coloured_graph[1].data(), sizeof(colour_t)*T );
	bsp_sync();

	// input matrix
	input_matrix( p, coloured_graph[parity] );
	
	bool unstable = true;
	while( unstable ) {
		// choose which matrix to read to and write from
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

			// share new edge colour with all processors (can be optimised for specifics distributions)
			for( int q = 0; q < P; ++q )
				if( q != p )
					bsp_put( q, newM.data()+index, newM.data(), index*sizeof( colour_t ), sizeof( colour_t ) );
		}

		// synchronise
		bsp_sync();

		// complete the matrix from upper triangle form
		for( int y = 0; y < N; ++y )
			for( int x = y+1; x < N; ++x )
				newM[m_index( x, y )] = newM[m_index( y, x )].reverse();

		// count the colours to check for stability
		for( int i = 0; i < T; ++i )
			colours.insert( newM[i] );
		if( colours.size() != relation_count )
			unstable = true;
		relation_count = colours.size();

		// clean-up and bookkeeping
		parity = not parity;
		iterations++;
		colours.clear();
	}

	// output matrix
	output_matrix( p, coloured_graph[parity] );

	// following is not part of the official output of this program, but is informative
	if( p == 0 )
		cerr << endl << "iterations: " << iterations << endl;

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
	T = N*N;
	processor_main();
	return 0;
}