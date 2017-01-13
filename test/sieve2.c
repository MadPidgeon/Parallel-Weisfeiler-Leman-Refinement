// gcc -o sieve1 sieve1.c lib/libmcbsp1.2.0.a -pthread -lrt -lm
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mcbsp.h"
#include "mcbsp-affinity.h"
#include <math.h>

typedef int prime_t;
typedef char byte;

int P;
int M;
int N;
int W;
int Q;

int min( int a, int b ) {
	return a<b?a:b;
}

int max( int a, int b ) {
	return a>b?a:b;
}

int binarysearch( int arr[], int a, int b, int t ) {
	if( b-a < 2 ) {
		if( arr[a] >= t ) return a;
		return b;
	}
	int middle = (a+b)/2;
	if( t < arr[middle] ) {
		return binarysearch( arr, a, middle-1, t );
	} else if( t > arr[middle] )
		return binarysearch( arr, middle+1, b, t);
	else
		return middle;
}

// count numbers in [a,b)
int count( int arr[], int n, int a, int b ) {
	if( a > arr[n-1] ) return 0;
	if( b < arr[0] ) return 0;

	int low = binarysearch( arr, 0, n-1, a);
	int high = binarysearch( arr, 0, n-1, b);
	// printf("hl %d %d\n", arr[high], arr[low] );

	if( arr[high] >= b ) high--;
	if( arr[low] < a ) low++;
	return high-low+1;
}


void spmd() {
	// init
	bsp_begin( P );
	int proc = bsp_pid();

	byte* sieve = malloc( sizeof(byte)*M );
	memset( sieve, 0, sizeof(byte)*M );

	prime_t base = 2*M*proc + 1;
	prime_t* primes = malloc( sizeof(prime_t)*M );
	int top = -1;

	prime_t bounds[6] = { -1,0,0,0,0,0};
	enum{
		in_lower,in_upper,in_sum,out_lower,out_upper,out_sum
	};
	bsp_push_reg( &bounds, 6*sizeof(prime_t) );

	prime_t prime[2] = {-1,-1};
	bsp_push_reg( &prime, 2*sizeof(prime_t) );

	bsp_sync();

	if( proc == 0 ) {
		// find all primes
		primes[++top] = 2;
		int mx = (sqrt(N)-0.5)/2.0;

		for( int i = 1; i <= mx; ++i ) { // for each i <= sqrt(N)
			if( !sieve[i] ) { // if 2*i+1 is prime
				// store 2*i+1
				primes[++top] = 2*i+1;
				prime[1] = 2*i+1;
				if( P > 1 ) { // if there are other processors, delegate
					bsp_put( 1, prime+1, prime, 0, sizeof(prime_t) );
					bsp_sync();
				}
				// mark other multiples of 2*i+1 as non-prime
				for( prime_t j = i; j < M; j += prime[1] )
					sieve[j] = 1;
			}
		}

		for( int i = mx+1; i < M; i++ ) { // for all sqrt(N) < i < M
			if( !sieve[i] ) { // if 2*i+1 is not marked as non-prime
				// 2*i+1 is prime
				primes[++top] = 2*i+1;
			}
		}

		// read bounds from stdin
		prime_t lower[Q];
		prime_t upper[Q];
		for( int i = 0; i < Q; i++ ) {
			scanf("%d", lower+i);
			scanf("%d", upper+i);
		}

		/*for( int i = 0; i <= top; ++i )
			printf("%d ",primes[i]);
		printf("\n");*/

		// count the primes in the bounds
		for( int i = 0; i < Q; i++ ) { // for each pair of bounds
			// printf("F %d %d\n", proc, bounds[in_lower]);
			if( bounds[in_lower] != -1 ) { // if we received the result of a query
				printf( "%d\n", bounds[in_sum] );
			}
			// count processor 0's primes in this bound
			bounds[out_sum] = count( primes, top+1, lower[i], upper[i] );
			if( P > 1 ) { // if there are other processors, delegate
				bounds[out_lower] = lower[i];
				bounds[out_upper] = upper[i];
				bsp_put( 1, bounds+3, bounds, 0, 3*sizeof(prime_t) );
				bsp_sync();
			} else { // else, output the result directly
				printf( "%d\n", bounds[out_sum] );
			}
		}

		// flush pipeline
		if( P > 1 ) {
			bounds[out_lower] = -1;
			bsp_put( 1, bounds+3, bounds, 0, 3*sizeof(prime_t) );
			while( bounds[in_lower] == -1 ) bsp_sync();
			while( bounds[in_lower] != -1 ) {
				printf( "%d\n", bounds[in_sum] );
				bsp_sync();
			}
		}
	} else {
		// wait for the pipeline to fill
		for( int i = 0; i < proc; ++i ) bsp_sync();
		while( bounds[in_lower] == -1 ) {
			// mark all multiples as non-prime
			int x = (prime[0] - (base % prime[0]) ) % prime[0];
			int start;
			if( x % 2 == 0 )
				start = x/2;
			else
				start = (x+prime[0])/2;
			for( int j = start; j < M; j += prime[0] ) sieve[j] = 1; 
			// propagate information
			prime[1] = prime[0];
			if( proc < P-1 )
				bsp_put( proc+1, prime+1, prime, 0, sizeof(prime_t) );
			bsp_sync();
		}

		// collect the primes in this processors interval
		for( int i = 0; i < M; ++i )
			if( !sieve[i] ) 
				primes[++top] = 2*(M*proc+i)+1;
		/*for( int i = 0; i <= top; ++i )
			printf("%d ",primes[i]);
		printf("\n");*/

		while( bounds[in_lower] != -1 ) {
			// count the primes in the received intervals
			bounds[out_lower] = bounds[in_lower];
			bounds[out_upper] = bounds[in_upper];
			bounds[out_sum] = bounds[in_sum];
			bounds[out_sum] += count( primes, top+1, bounds[out_lower], bounds[out_upper] );

			// propagate information
			bsp_put( (proc+1)%P, bounds+3, bounds, 0, 3*sizeof(prime_t) );
			bsp_sync();
		}

		// flush the pipeline
		bounds[out_lower] = -1;
		for( int i = 0; i < P - proc; ++i ) {
			bsp_put( (proc+1)%P, bounds+3, bounds, 0, 3*sizeof(prime_t) );
			bsp_sync();
		}
	}

	bsp_end();
}

int main( int argc, char ** argv ) {
	bsp_init( &spmd, argc, argv );
	mcbsp_set_affinity_mode(COMPACT);
	/*if( argc != 3 ) {
		printf("usage: sieve n p\n  n: find primes up to n\n  p: use p processors\n");
		return EXIT_FAILURE;
	}*/
	scanf("%d", &P);
	scanf("%d", &N);
	scanf("%d", &Q);
	N = max( N, P*P );
	W = 2*P;
	N += (N%W == 0 ) ? 0 : W-(N%W);
	M = N / W;
	spmd();
	return EXIT_SUCCESS;
}
