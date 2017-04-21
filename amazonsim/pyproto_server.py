from google.protobuf.internal.decoder import _DecodeVarint32
from google.protobuf.internal.encoder import _EncodeVarint
import internalcom_pb2
import socket
import struct

print("server: single thread started...")
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(("localhost", 12345))
# become a server socket
sock.listen(5)
clientSocket, addr = sock.accept()
read_size = clientSocket.recv(8)
read_size_unpacked = struct.unpack("!q", read_size)[0]
print ("server: expecting protobuf of size {0}".format(read_size_unpacked))
received_proto_bytes = clientSocket.recv(read_size_unpacked)
received_proto = internalcom_pb2.Order()
received_proto.ParseFromString(received_proto_bytes)
print("received tracking number: {0}".format(received_proto.shipid))
