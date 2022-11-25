#include <boost/algorithm/string/predicate.hpp>
#include <boost/program_options.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "json.hpp"

#include "sample_pipeline.h"
#include "sample_writer.h"

using json = nlohmann::json;
namespace po = boost::program_options;

std::string uhd_args, file, fft_file, type, ant, subdev, ref, wirefmt;
size_t channel, total_num_samps, spb, zlevel, rate, nfft, nfft_overlap, nfft_div, nfft_ds, batches, sample_id;
double option_rate, freq, gain, bw, total_time, setup_time, lo_offset;
bool null, fftnull, use_vkfft, use_json_args, int_n, skip_lo;
static bool stop_streaming;
po::variables_map vm;


bool check_sensor_lock(std::vector<std::string> sensor_names, const std::string& sensor_name, std::function<uhd::sensor_value_t(const std::string&)> get_sensor_fn, double setup_time)
{
    if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name) == sensor_names.end())
        return false;

    auto setup_timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(int64_t(setup_time * 1000));
    bool lock_detected = false;

    std::cerr << boost::format("waiting for \"%s\" lock: ") % sensor_name;
    std::cerr.flush();

    while (std::chrono::steady_clock::now() < setup_timeout)
        {
            if (!lock_detected)
                {
                    lock_detected = get_sensor_fn(sensor_name).to_bool();
                }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    if (!lock_detected)
        {
            throw std::runtime_error(
                                     str(boost::format("timed out waiting for lock on sensor \"%s\"") % sensor_name));
        }
    return true;
}


void lo_lock(uhd::usrp::multi_usrp::sptr usrp, std::string &ref, size_t channel, double setup_time)
{
    check_sensor_lock(usrp->get_rx_sensor_names(channel),
                      "lo_locked",
                      [usrp, channel](const std::string& sensor_name) {
                          return usrp->get_rx_sensor(sensor_name, channel);
                      },
                      setup_time);
    std::string ref_name = "";
    if (ref == "mimo")
        {
            ref_name = "mimo_locked";
        }
    if (ref == "external")
        {
            ref_name = "ref_locked";
        }
    if (ref_name != "")
        {
            check_sensor_lock(usrp->get_mboard_sensor_names(0),
                              ref_name,
                              [usrp](const std::string& sensor_name) {
                                  return usrp->get_mboard_sensor(sensor_name);
                              },
                              setup_time);
        }
}


void tune(uhd::usrp::multi_usrp::sptr usrp, size_t channel, double freq, double lo_offset, bool int_n)
{
    uhd::tune_request_t tune_request(freq, lo_offset);
    if (int_n)
        {
            tune_request.args = uhd::device_addr_t("mode_n=integer");
        }
    usrp->set_rx_freq(tune_request, channel);
    std::cerr << boost::format("Set RX freq %f MHz with LO offset %f MHz, got actual RX freq: %f MHz...")
        % (freq / 1e6) % (lo_offset / 1e6) % (usrp->get_rx_freq(channel / 1e6))
              << std::endl;
}


void sig_int_handler(int)
{
    stop_streaming = true;
}


bool run_stream(uhd::rx_streamer::sptr rx_stream, double time_requested, size_t max_samples, size_t num_requested_samples)
{
    bool overflows = false;
    size_t write_ptr = 0;
    size_t num_total_samps = 0;
    const auto stop_time =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(int64_t(1000 * time_requested));
    stop_streaming = false;

    for (;;)
        {
            uhd::rx_metadata_t md;
            size_t buffer_capacity = 0;
            char *buffer_p = get_sample_buffer(write_ptr, &buffer_capacity);
            size_t num_rx_samps = rx_stream->recv(buffer_p, max_samples, md, 3.0, false);

            switch (md.error_code)
                {
                case uhd::rx_metadata_t::ERROR_CODE_NONE:
                    break;
                case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
                    std::cerr << "ERROR_CODE_TIMEOUT" << std::endl;
                    stop_streaming = true;
                    break;
                case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
                    std::cerr << "ERROR_CODE_OVERFLOW" << std::endl;
                    overflows = true;
                    stop_streaming = true;
                    break;
                default:
                    stop_streaming = true;
                    std::cerr << md.strerror() << std::endl;
                    break;
                }

            num_total_samps += num_rx_samps;
            size_t samp_bytes = num_rx_samps * get_samp_size();
            if (samp_bytes != buffer_capacity) {
                std::cerr << "resize to " << samp_bytes << " from " << buffer_capacity << std::endl;
                set_sample_buffer_capacity(write_ptr, samp_bytes);
            }

            enqueue_samples(write_ptr);

            if (stop_streaming)
                break;
            if (num_requested_samples and num_requested_samples >= num_total_samps)
                break;
            if (time_requested and std::chrono::steady_clock::now() >= stop_time)
                break;
        }

    return overflows;
}


void sample_record(uhd::usrp::multi_usrp::sptr usrp,
                   const std::string& type,
                   const std::string& wire_format,
                   const size_t& channel,
                   const std::string& file,
                   const std::string& fft_file,
                   const size_t rate,
                   const size_t samps_per_buff,
                   const size_t zlevel,
                   const size_t num_requested_samples,
                   const double time_requested,
                   const bool use_vkfft,
                   const size_t nfft,
                   const size_t nfft_overlap,
                   const size_t nfft_div,
                   const size_t nfft_ds,
                   const size_t batches,
                   const size_t sample_id)
{
    std::string cpu_format;
    set_sample_pipeline_types(type, cpu_format);

    uhd::stream_args_t stream_args(cpu_format, wire_format);
    std::vector<size_t> channel_nums;
    channel_nums.push_back(channel);
    stream_args.channels = channel_nums;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    const size_t max_samps_per_packet = rx_stream->get_max_num_samps();
    const size_t max_samples = std::max(max_samps_per_packet, samps_per_buff);
    std::cerr << "max_samps_per_packet from stream: " << max_samps_per_packet << std::endl;

    if (nfft) {
        std::cerr << "using FFT point size " << nfft << std::endl;

        if (samps_per_buff % nfft) {
            throw std::runtime_error("FFT point size must be a factor of spb");
        }
    }

    sample_pipeline_start(file, fft_file,
                          max_samples, zlevel,
                          use_vkfft, nfft,
                          nfft_overlap, nfft_div, nfft_ds,
                          rate, batches, sample_id);

    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                 ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                 : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = size_t(num_requested_samples);
    stream_cmd.stream_now = true;
    stream_cmd.time_spec  = uhd::time_spec_t();
    rx_stream->issue_stream_cmd(stream_cmd);

    bool overflows = run_stream(rx_stream, time_requested, max_samples, num_requested_samples);

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);
    std::cerr << "stream stopped" << std::endl;
    sample_pipeline_stop(overflows);
    std::cerr << "pipeline stopped" << std::endl;
}


int parse_args(int argc, char* argv[])
{
    // compatible with uhd_rx_samples_to_file.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&uhd_args)->default_value(""), "multi uhd device address args")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat.zst"), "name of the file to write binary samples to")
        ("type", po::value<std::string>(&type)->default_value("short"), "sample type: double, float, or short")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
        ("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
        ("zlevel", po::value<size_t>(&zlevel)->default_value(1), "default compression level")
        ("spb", po::value<size_t>(&spb)->default_value(0), "samples per buffer (if 0, same as rate)")
        ("rate", po::value<double>(&option_rate)->default_value(2.048e6), "rate of incoming samples")
        ("freq", po::value<double>(&freq)->default_value(100e6), "RF center frequency in Hz")
        ("lo-offset", po::value<double>(&lo_offset)->default_value(0.0),
         "Offset for frontend LO in Hz (optional)")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("ant", po::value<std::string>(&ant), "antenna selection")
        ("subdev", po::value<std::string>(&subdev), "subdevice specification")
        ("channel", po::value<size_t>(&channel)->default_value(0), "which channel to use")
        ("bw", po::value<double>(&bw), "analog frontend filter bandwidth in Hz")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "reference source (internal, external, mimo)")
        ("wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8, sc16)")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
        ("null", "run without writing to file")
        ("fftnull", "run without writing to FFT file")
        ("skip-lo", "skip checking LO lock status")
        ("int-n", "tune USRP with integer-N tuning")
        ("nfft", po::value<size_t>(&nfft)->default_value(0), "if > 0, calculate n FFT points")
        ("nfft_overlap", po::value<size_t>(&nfft_overlap)->default_value(0), "FFT overlap")
        ("nfft_div", po::value<size_t>(&nfft_div)->default_value(50), "calculate FFT over sample rate / n samples (e.g 50 == 20ms)")
        ("nfft_ds", po::value<size_t>(&nfft_ds)->default_value(1), "NFFT downsampling interval")
        ("fft_file", po::value<std::string>(&fft_file)->default_value(""), "name of file to write FFT points to (default derive from --file)")
        ("novkfft", "do not use vkFFT (use software FFT)")
        ("vkfft_batches", po::value<size_t>(&batches)->default_value(100), "vkFFT batches")
        ("vkfft_sample_id", po::value<size_t>(&sample_id)->default_value(0), "vkFFT sample_id")
        ("json", "take parameters from json on stdin");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    null = vm.count("null") > 0;
    fftnull = vm.count("fftnull") > 0;
    use_vkfft = vm.count("novkfft") == 0;
    use_json_args = vm.count("json") > 0;
    int_n = vm.count("int-n") > 0;
    skip_lo = vm.count("skip-lo") > 0;

    if (vm.count("help")) {
        std::cerr << boost::format("uhd_sample_recorder: %s") % desc << std::endl;
        return ~0;
    }

    if (!boost::algorithm::starts_with(wirefmt, "sc")) {
        throw std::runtime_error("non-complex wirefmt not supported");
    }

    if (option_rate <= 0.0) {
        throw std::runtime_error("invalid sample rate");
    }
    rate = size_t(option_rate);

    if (rate % nfft_div) {
        throw std::runtime_error("nfft_div must be a factor of sample rate");
    }

    if (spb == 0) {
        spb = rate;
        std::cerr << "defaulting spb to rate (" << spb << ")" << std::endl;
    }

    if (!fft_file.size()) {
        fft_file = get_prefix_file(file, "fft_");
    }

    if (null) {
        file.clear();
    }

    if (fftnull || nfft == 0) {
        fft_file.clear();
    }

    return 0;
}


void serve_json(uhd::usrp::multi_usrp::sptr usrp)
{
    json status, json_args;
    std::string line, last_error;
    for (;;) {
        status["freq"] = freq;
        status["last_error"] = last_error;
        last_error.clear();
        std::cout << status << std::endl;
        json_args.clear();
        if (!std::getline(std::cin, line)) {
            break;
        }
        try {
            json_args = json::parse(line);
        }
        catch (json::parse_error& ex) {
            last_error = "json parser error";
            continue;
        }
        try {
            file = json_args.value("file", file);
            fft_file = json_args.value("fft_file", fft_file);
            total_time = json_args.value("duration", total_time);
            freq = json_args.value("freq", freq);
            nfft = json_args.value("nfft", nfft);
            nfft_overlap = json_args.value("nfft_overlap", nfft_overlap);
        }
        catch (json::basic_json::type_error &ex) {
            last_error = "json parameter type error";
            continue;
        }
        tune(usrp, channel, freq, lo_offset, int_n);
        if (!skip_lo) {
            lo_lock(usrp, ref, channel, setup_time);
        }
        sample_record(usrp, type, wirefmt, channel,
                      file, fft_file, rate, spb, zlevel, total_num_samps, total_time,
                      use_vkfft, nfft, nfft_overlap, nfft_div, nfft_ds, batches, sample_id);
        last_error = "";
    }
}


void serve_once(uhd::usrp::multi_usrp::sptr usrp)
{
    if (total_num_samps == 0) {
        std::cerr << "^C to stop" << std::endl;
    }

    sample_record(usrp, type, wirefmt, channel,
                  file, fft_file, rate, spb, zlevel, total_num_samps, total_time,
                  use_vkfft, nfft, nfft_overlap, nfft_div, nfft_ds, batches, sample_id);
}


void init_usrp(uhd::usrp::multi_usrp::sptr usrp)
{
    std::cerr << boost::format("using: %s") % usrp->get_pp_string() << std::endl;

    if (vm.count("ref")) {
        usrp->set_clock_source(ref);
    }

    if (vm.count("subdev"))
        usrp->set_rx_subdev_spec(subdev);

    if (vm.count("ant"))
        usrp->set_rx_antenna(ant, channel);

    std::cerr << boost::format("setting RX rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_rx_rate(rate, channel);
    std::cerr << boost::format("actual RX rate: %f Msps...") % (usrp->get_rx_rate(channel) / 1e6) << std::endl;

    if (vm.count("gain")) {
        std::cerr << boost::format("setting RX gain: %f dB...") % gain << std::endl;
        usrp->set_rx_gain(gain, channel);
        std::cerr << boost::format("actual RX gain: %f dB...") % usrp->get_rx_gain(channel) << std::endl;
    }

    if (vm.count("bw")) {
        std::cerr << boost::format("setting RX bandwidth: %f MHz...") % (bw / 1e6) << std::endl;
        usrp->set_rx_bandwidth(bw, channel);
        std::cerr << boost::format("actual RX bandwidth: %f MHz...") % (usrp->get_rx_bandwidth(channel) / 1e6) << std::endl;
    }

    tune(usrp, channel, freq, lo_offset, int_n);
    if (!skip_lo) {
        lo_lock(usrp, ref, channel, setup_time);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(1000 * setup_time)));
}


int UHD_SAFE_MAIN(int argc, char* argv[])
{
    if (parse_args(argc, argv))
        return ~0;

    std::cerr << boost::format("creating usrp device with: %s...") % uhd_args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(uhd_args);
    init_usrp(usrp);
    std::signal(SIGINT, &sig_int_handler);

    if (use_json_args) {
        serve_json(usrp);
    } else {
        serve_once(usrp);
    }

    return EXIT_SUCCESS;
}
