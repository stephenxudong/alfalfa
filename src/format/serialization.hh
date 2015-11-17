#ifndef SERIALIZATION_HH
#define SERIALIZATION_HH

#include <vector>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "optional.hh"
#include "frame_info.hh"
#include "dependency_tracking.hh"
#include "protobufs/alfalfa.pb.h"
#include "file_descriptor.hh"

struct VideoInfo
{
  std::string fourcc;
  uint16_t width;
  uint16_t height;

  VideoInfo()
    : fourcc( "VP80" ), width( 0 ), height( 0 )
  {}

  VideoInfo( std::string fourcc, uint16_t width, uint16_t height )
    : fourcc( fourcc ), width( width ), height( height )
  {}

  bool operator==( const VideoInfo & other ) const
  {
    return ( fourcc == other.fourcc and
      width == other.width and
      height == other.height
    );
  }

  bool operator!=( const VideoInfo & other ) const
  {
    return not ( ( *this ) == other );
  }
};

struct RasterData
{
  size_t hash;
};

struct QualityData
{
  size_t original_raster;
  size_t approximate_raster;
  double quality;
};

struct TrackData
{
  size_t track_id;
  size_t frame_index;
  size_t frame_id;

  TrackData ( const size_t track_id, const size_t frame_index,
              const size_t frame_id ) : track_id( track_id ),
                                        frame_index( frame_index ),
                                        frame_id( frame_id )
  {}

  TrackData( const size_t track_id, const size_t frame_id )
    : track_id( track_id ),
      frame_index( 0 ),
      frame_id( frame_id )
  {}

  TrackData()
    : track_id(0), frame_index(0), frame_id(0)
  {}
};

struct SwitchData
{
  size_t from_track_id;
  size_t from_frame_index;
  size_t to_track_id;
  size_t to_frame_index;
  size_t frame_id;
  size_t switch_frame_index;

  SwitchData( const size_t & from_track_id, const size_t & from_frame_index,
    const size_t & to_track_id, const size_t & to_frame_index,
    const size_t & frame_id, const size_t & switch_frame_index )
    : from_track_id( from_track_id ),
      from_frame_index( from_frame_index ),
      to_track_id( to_track_id ),
      to_frame_index( to_frame_index ),
      frame_id( frame_id ),
      switch_frame_index( switch_frame_index )
  {}

  SwitchData()
    : from_track_id(0), from_frame_index(0), to_track_id(0), to_frame_index(0),
      frame_id(0), switch_frame_index(0)
  {}
};

AlfalfaProtobufs::VideoInfo   to_protobuf( const VideoInfo & info );
AlfalfaProtobufs::SourceHash  to_protobuf( const SourceHash & source_hash );
AlfalfaProtobufs::TargetHash  to_protobuf( const TargetHash & target_hash );
AlfalfaProtobufs::RasterData  to_protobuf( const RasterData & rd );
AlfalfaProtobufs::QualityData to_protobuf( const QualityData & qd );
AlfalfaProtobufs::TrackData   to_protobuf( const TrackData & td );
AlfalfaProtobufs::FrameInfo   to_protobuf( const FrameInfo & fi );
AlfalfaProtobufs::SwitchData  to_protobuf( const SwitchData &sd );

VideoInfo   from_protobuf( const AlfalfaProtobufs::VideoInfo & message );
SourceHash  from_protobuf( const AlfalfaProtobufs::SourceHash & message );
TargetHash  from_protobuf( const AlfalfaProtobufs::TargetHash & message );
RasterData  from_protobuf( const AlfalfaProtobufs::RasterData & item );
QualityData from_protobuf( const AlfalfaProtobufs::QualityData & message );
TrackData   from_protobuf( const AlfalfaProtobufs::TrackData & message );
FrameInfo   from_protobuf( const AlfalfaProtobufs::FrameInfo & pfi );
SwitchData  from_protobuf( const AlfalfaProtobufs::SwitchData &pwd );

// end of protobuf converters

class ProtobufSerializer
{
protected:
  FileDescriptor fout_;
  google::protobuf::io::FileOutputStream raw_output_ { fout_.num() };
  google::protobuf::io::CodedOutputStream coded_output_ { &raw_output_ };

public:
  ProtobufSerializer( const std::string & filename );
  void write_string( const std::string & str );

  template<class EntryProtobufType>
  bool write_protobuf( const EntryProtobufType & entry );
};

class ProtobufDeserializer
{
protected:
  FileDescriptor fin_;
  google::protobuf::io::FileInputStream raw_input_ { fin_.num() };
  google::protobuf::io::CodedInputStream coded_input_ { &raw_input_ };

public:
  ProtobufDeserializer( const std::string & filename );
  std::string read_string( const size_t size );

  template<class EntryProtobufType>
  bool read_protobuf( EntryProtobufType & protobuf );
};

template<class EntryProtobufType>
bool ProtobufDeserializer::read_protobuf( EntryProtobufType & message )
{
  google::protobuf::uint32 size;
  bool has_next = coded_input_.ReadLittleEndian32( &size );

  if ( not has_next ) {
    return false;
  }

  google::protobuf::io::CodedInputStream::Limit message_limit =
    coded_input_.PushLimit( size );

  if ( message.ParseFromCodedStream( &coded_input_ ) ) {
    coded_input_.PopLimit( message_limit );
    return true;
  }

  return false;
}

template<class EntryProtobufType>
bool ProtobufSerializer::write_protobuf( const EntryProtobufType & protobuf )
{
  coded_output_.WriteLittleEndian32( protobuf.ByteSize() );
  return protobuf.SerializeToCodedStream( &coded_output_ );
}

#endif /* SERIALIZATION_HH */
