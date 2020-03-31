//
// A wrapper for JPEG reading.
//


#ifndef RTCW_JPEG_READER_INCLUDED
#define RTCW_JPEG_READER_INCLUDED


#include <string>
#include <vector>
#include "jpgd.h"


namespace rtcw {


class JpegReader {
public:
    JpegReader();

    ~JpegReader();

    JpegReader(
        const JpegReader& that) = delete;

    JpegReader& operator=(
        const JpegReader& that) = delete;


    bool open(
        const void* src_data,
        int src_size,
        int& width,
        int& height);

    bool decode(
        void* dst_data);

    void close();

    int get_width() const;

    int get_height() const;

    const std::string& get_error_message() const;


private:
    int width_;
    int height_;
    bool is_grayscale_;
    jpgd::jpeg_decoder_mem_stream* stream_;
    jpgd::jpeg_decoder* decoder_;
    std::string error_message_;


    static void gray_to_rgba(
        const unsigned char* src_row,
        int src_width,
        unsigned char* dst_row);
}; // JpegReader


} // rtcw


#endif // RTCW_JPEG_READER_INCLUDED