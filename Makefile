CC=g++
CFLAGS=-std=c++14 -g
EXTRAFLAGS=`pkg-config --cflags --libs protobuf`

#all: protobuf_test protobuf_test_read client2
all: amazon_server internalClientTest 

protobuf_test: protobuf_test.cpp addressbook.pb.cc
	{ \
	SRC_DIR=".";\
	DST_DIR=".";\
	export SRC_DIR;\
	export DST_DIR;\
	protoc -I=$$SRC_DIR --cpp_out=$$DST_DIR $$SRC_DIR/addressbook.proto;\
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o protobuf_test protobuf_test.cpp addressbook.pb.cc $(EXTRAFLAGS);\
	}

protobuf_test_read: protobuf_test.cpp addressbook.pb.cc
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o protobuf_test_read protobuf_test_read.cpp addressbook.pb.cc $(EXTRAFLAGS)

server: server.cpp addressbook.pb.cc utility.cpp
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o server server.cpp addressbook.pb.cc utility.cpp $(EXTRAFLAGS)

client: client.cpp addressbook.pb.cc utility.cpp
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o client client.cpp addressbook.pb.cc utility.cpp $(EXTRAFLAGS)

proto:
	{ \
	SRC_DIR=".";\
	DST_DIR="./amazonsim";\
	export SRC_DIR;\
	export DST_DIR;\
	protoc -I=$$SRC_DIR --python_out=$$DST_DIR $$SRC_DIR/internalcom.proto;\
	}

amazon_server:
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o amazon_server utility.cpp amazon_server.cpp amazon.pb.cc internalcom.pb.cc ig.pb.cc nonBlockingIO/sleepyProtobuff.cc nonBlockingIO/ups_comm.cc dbHandler.cc -lpqxx -lpq $(EXTRAFLAGS)

internalClientTest:
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o internalClientTest utility.cpp nonBlockingIO/internalClientTest.cc internalcom.pb.cc ig.pb.cc  $(EXTRAFLAGS)

#ups_messenger:
#	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o ups_messenger utility.cpp nonBlockingIO/ups_comm.cc ig.pb.cc dbHandler.cc -lpqxx -lpq $(EXTRAFLAGS)
