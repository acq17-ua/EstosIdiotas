// Example program
#include <iostream>
#include <string>
using namespace std;
int main()
{
    unsigned char* map[5];
	unsigned char a = (unsigned char)64, b=(unsigned char)256;
	map[0] = &a;
	map[1] = &a;
	map[2] = &a;
	map[3] = &a;
	map[4] = &b;
	
	for( int i=0; i<5; i++ ) {
		cout << (int)*map[i] << "\t";
	}
	cout << endl;

	return 0;
}