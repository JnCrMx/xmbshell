/* XMBShell, a console-like desktop shell
 * Copyright (C) 2025 - JCM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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

    using ::avcodec_license;
    using ::avdevice_license;
    using ::avfilter_license;
    using ::avformat_license;
    using ::avutil_license;
    using ::swresample_license;
    using ::swscale_license;
}
