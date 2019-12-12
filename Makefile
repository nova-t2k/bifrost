all:
	g++ -std=c++1y test_outside.cxx -o test_outside
	g++ -std=c++1y test_inside.cxx -o test_inside
