
#include <boost/algorithm/string/predicate.hpp>
#include <boost/program_options.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/program_options.hpp>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "json.hpp"

#include "sample_pipeline.h"
#include "sample_writer.h"

using json = nlohmann::json;
namespace po = boost::program_options;


int UHD_SAFE_MAIN(int argc, char* argv[])
{
	return EXIT_SUCCESS;
}
