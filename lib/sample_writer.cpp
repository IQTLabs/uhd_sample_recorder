#include "sample_writer.h"
#include <boost/filesystem.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zstd.hpp>
#include <iostream>

std::string get_prefix_file(const std::string &file,
                            const std::string &prefix) {
  boost::filesystem::path orig_path(file);
  std::string basename(orig_path.filename().c_str());
  std::string dirname(
      boost::filesystem::canonical(orig_path.parent_path()).c_str());
  return dirname + "/" + prefix + basename;
}

std::string get_dotfile(const std::string &file) {
  return get_prefix_file(file, ".");
}

SampleWriter::SampleWriter() {
  outbuf_p.reset(new boost::iostreams::filtering_ostream());
}

void SampleWriter::write(const char *data, size_t len) {
  if (!outbuf_p->empty()) {
    outbuf_p->write(data, len);
  }
}

void SampleWriter::open(const std::string &file, size_t zlevel) {
  file_ = file;
  dotfile_ = get_dotfile(file_);
  orig_path_ = boost::filesystem::path(file_);
  std::cerr << "opening " << dotfile_ << std::endl;
  if (orig_path_.has_extension()) {
    if (orig_path_.extension() == ".gz") {
      std::cerr << "writing gzip compressed output" << std::endl;
      outbuf_p->push(boost::iostreams::gzip_compressor(
          boost::iostreams::gzip_params(zlevel)));
    } else if (orig_path_.extension() == ".zst") {
      std::cerr << "writing zstd compressed output" << std::endl;
      outbuf_p->push(boost::iostreams::zstd_compressor(
          boost::iostreams::zstd_params(zlevel)));
    } else {
      std::cerr << "writing uncompressed output" << std::endl;
    }
  }
  outbuf_p->push(boost::iostreams::file_sink(dotfile_));
}

void SampleWriter::close(size_t overflows) {
  if (!outbuf_p->empty()) {
    std::cerr << "closing " << file_ << std::endl;
    outbuf_p->reset();

    if (overflows) {
      std::string dirname(
          boost::filesystem::canonical(orig_path_.parent_path()).c_str());
      std::string overflow_name = dirname + "/overflow-" + file_;
      rename(dotfile_.c_str(), overflow_name.c_str());
    } else {
      rename(dotfile_.c_str(), file_.c_str());
    }
  }
}
