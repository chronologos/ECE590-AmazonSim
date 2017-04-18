#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

template<typename T> bool sendMesgTo(const T & message,
                                     google::protobuf::io::FileOutputStream *out);
template<typename T> bool recvMesgFrom(T & message,
                                       google::protobuf::io::FileInputStream * in );
std::ostream & operator<<(std::ostream & s, const google::protobuf::Message & m);
