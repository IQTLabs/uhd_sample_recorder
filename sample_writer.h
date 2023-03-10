#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/scoped_ptr.hpp>

#ifndef SAMPLE_WRITER_H
#define SAMPLE_WRITER_H 1
class SampleWriter
{
public:
    SampleWriter();
    void open(const std::string &file, size_t zlevel);
    void close(size_t overflows);
    void write(const char *data, size_t len);
private:
    boost::scoped_ptr<boost::iostreams::filtering_ostream> outbuf_p;
    std::string file_;
    std::string dotfile_;
    boost::filesystem::path orig_path_;
};


std::string get_prefix_file(const std::string &file, const std::string &prefix);
#endif
