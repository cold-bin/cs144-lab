#include "byte_stream.hh"
#include <cmath>
#include <stdexcept>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : cap( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  if ( error_ || is_closed() || data.empty() ) {
    return;
  }

  uint64_t const len = min( data.length(), available_capacity() );
  buf.append( data.substr( 0, len ) );
  num_of_write += len;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

void Writer::set_error()
{
  // Your code here.
  error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return cap - num_of_write + num_of_read;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return num_of_write;
}

string_view Reader::peek() const
{
  // Your code here.
  return { buf };
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && buf.empty();
}

bool Reader::has_error() const
{
  // Your code here.
  return error_;
}

void Reader::pop( uint64_t len )
{
  if ( buf.empty() ) {
    return;
  }

  len = std::min( len, buf.size() );
  buf.erase(buf.begin(), buf.begin() + static_cast<int64_t>( len ) );
  num_of_read += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  // pushed + not poped(left)
  return num_of_write - num_of_read;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return num_of_read;
}