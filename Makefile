all:
	g++ -pthread -O3 -s console2.cpp -o console.cgi -lboost_system
	g++ -pthread -o http_server http_server.cpp -lboost_system