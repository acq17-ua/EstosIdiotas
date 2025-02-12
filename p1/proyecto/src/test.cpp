#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
using namespace std;

int main(int argc, char** argv) {

	string filename = "tester.txt";
	ostringstream oss;
	string output;

	auto start = std::chrono::high_resolution_clock::now();

	for( int i=0; i<100; i++ ) {
		if (oss << std::ifstream(filename, std::ios::binary).rdbuf())
			output = oss.str();
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);

	cout << elapsed << endl;

	return 0;
}
