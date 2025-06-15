module;

#include "audioresampler.h"
#include "av.h"
#include "avutils.h"
#include "averror.h"
#include "codec.h"
#include "ffmpeg.h"
#include "packet.h"
#include "videorescaler.h"

// API2
#include "codec.h"
#include "codeccontext.h"
#include "filters/buffersink.h"
#include "filters/buffersrc.h"
#include "filters/filtergraph.h"
#include "format.h"
#include "formatcontext.h"

export module avcpp;

export namespace av {
    using av::AudioDecoderContext;
    using av::AudioEncoderContext;
    using av::AudioResampler;
    using av::AudioSamples;
    using av::BufferSinkFilterContext;
    using av::BufferSrcFilterContext;
    using av::Codec;
    using av::CodecContext2;
    using av::CodecParameters;
    using av::Errors;
    using av::Filter;
    using av::FilterGraph;
    using av::findDecodingCodec;
    using av::findEncodingCodec;
    using av::FormatContext;
    using av::init;
    using av::OutputFormat;
    using av::Packet;
    using av::PixelFormat;
    using av::set_logging_level;
    using av::Stream;
    using av::VideoDecoderContext;
    using av::VideoEncoderContext;
    using av::VideoFrame;
    using av::VideoRescaler;
    using av::OptionalErrorCode;
    using av::Errors;
    using av::error2string;
    using av::make_ffmpeg_error;
}
