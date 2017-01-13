#include "city.h"
#include <bits/stdc++.h>

using namespace std;

typedef std::pair<uint64, uint64> colour_t;

string int64ToString(int64_t n) {
  string a(8,0);
  memcpy(&a[0], &n, 8);
  return a;
}

// assumes b is already sorted
colour_t hasher( colour_t a, vector<colour_t> b ) {
	string str = int64ToString( a.first ) + int64ToString( a.second );
	for( int i = 0; i < b.size(); i++ ) {
		str += int64ToString( b[i].first ) + int64ToString( b[i].second );
	}
	return CityHash128( str.c_str(), 8 + b.size()*8 );
}

int main (){

	colour_t test = { 323423ULL, 3423143ULL };
	cout << hasher( test, { test, test, test } ).first << endl;

	return 0;
}