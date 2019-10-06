//   rewrite oct 2019
// --------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <complex>

#include "TSDRPlugin.h"

#include "TSDRCodes.h"

#include <stdint.h>
#include <boost/algorithm/string.hpp>

#include "errors.hpp"

#define HOW_OFTEN_TO_CALL_CALLBACK_SEC (0.06)
#define FRACT_DROPPED_TO_TOLERATE (0)

uhd::usrp::multi_usrp::sptr usrp;
namespace po = boost::program_options;
volatile int is_running = 0;

EXTERNC TSDRPLUGIN_API void __stdcall tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR UHD USRP Compatible Plugin");
}

std::string device_args("");
std::string subdev("A:A");
std::string ant("TX/RX");
std::string ref("internal");

double rate(1e6);
double freq(915e6);
double gain(10);
double bw(1e6);

uint32_t req_freq = 105e6;
float req_gain = 1;
double req_rate = 25e6;

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_init(const char * params) {
    uhd::set_thread_priority_safe();

  try{
    std::string device_args("");
    std::string subdev("A:A");
    std::string ant("TX/RX");
    std::string ref("internal");

    double rate(1e6);
    double freq(915e6);
    double gain(10);
    double bw(1e6);

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % device_args << std::endl;
    usrp = uhd::usrp::multi_usrp::make(device_args);

    // Lock mboard clocks
    std::cout << boost::format("Lock mboard clocks: %f") % ref << std::endl;
    usrp->set_clock_source(ref);
    
    //always select the subdevice first, the channel mapping affects the other settings
    std::cout << boost::format("subdev set to: %f") % subdev << std::endl;
    usrp->set_rx_subdev_spec(subdev);

    //     std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //set the sample rate
    if (rate <= 0.0) {
        std::cerr << "Please specify a valid sample rate" << std::endl;
        return ~0;
    }

    // set sample rate
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate() / 1e6) << std::endl << std::endl;

    // set freq
    std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq / 1e6) << std::endl;
    uhd::tune_request_t tune_request(freq);
    usrp->set_rx_freq(tune_request);
    std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq() / 1e6) << std::endl << std::endl;

    // set the rf gain
    std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
    usrp->set_rx_gain(gain);
    std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain() << std::endl << std::endl;

    // set the IF filter bandwidth
    std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw / 1e6) << std::endl;
    usrp->set_rx_bandwidth(bw);
    std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % (usrp->get_rx_bandwidth() / 1e6) << std::endl << std::endl;

    // set the antenna
    std::cout << boost::format("Setting RX Antenna: %s") % ant << std::endl;
    usrp->set_rx_antenna(ant);
    std::cout << boost::format("Actual RX Antenna: %s") % usrp->get_rx_antenna() << std::endl << std::endl;
  }
  catch (std::exception const&  ex)
	{
		RETURN_EXCEPTION(ex.what(), TSDR_CANNOT_OPEN_DEVICE);
	}

	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API uint32_t __stdcall tsdrplugin_setsamplerate(uint32_t rate) {
	if (is_running)
		return tsdrplugin_getsamplerate();

	req_rate = rate;

	try {
		usrp->set_rx_rate(req_rate);
		double real_rate = usrp->get_rx_rate();
		req_rate = real_rate;
	}
	catch (std::exception const&  ex)
	{
	}

	return req_rate;
}

EXTERNC TSDRPLUGIN_API uint32_t __stdcall tsdrplugin_getsamplerate() {

	try {
		req_rate = usrp->get_rx_rate();
	}
	catch (std::exception const&  ex)
	{
	}

	return req_rate;
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_setbasefreq(uint32_t freq) {
	req_freq = freq;

	try {
		usrp->set_rx_freq(req_freq);
	}
	catch (std::exception const&  ex)
	{
	}

	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_stop(void) {
	is_running = 0;
	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

double tousrpgain(float gain)
{
  try
  {
    uhd::gain_range_t range = usrp->get_tx_gain_range();
    return gain * (range.stop() - range.start()) + range.start();
  }
  catch(std::exception const& ex)
  {
    return gain *60;
  }
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_setgain(float gain) {
	req_gain = gain;
	try {
		usrp->set_rx_gain(tousrpgain(req_gain));
	}
	catch (std::exception const&  ex)
	{
    std::cout << "tsdrplugin_setgain" << std::endl;
	}
	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	uhd::set_thread_priority_safe();

	is_running = 1;

	float * buff = NULL;

	try {
		//set the rx sample rate
		usrp->set_rx_rate(req_rate);

		//set the rx center frequency
		usrp->set_rx_freq(req_freq);

		//set the rx rf gain
		usrp->set_rx_gain(tousrpgain(req_gain));

		//create a receive streamer
		uhd::stream_args_t stream_args("fc32"); //complex floats
		uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

		//loop until total number of samples reached
		// 1 sample = 2 items
		uhd::rx_metadata_t md;
		md.has_time_spec = false;

		size_t buff_size = HOW_OFTEN_TO_CALL_CALLBACK_SEC * req_rate * 2;
		const size_t samples_per_api_read = rx_stream->get_max_num_samps();
		if (buff_size < samples_per_api_read * 2) buff_size = samples_per_api_read * 2;
		buff = (float *) malloc(sizeof(float) * buff_size);
		const size_t items_per_api_read = samples_per_api_read*2;

		// initialize counters
		size_t items_in_buffer = 0;

		const uint64_t samp_rate_uint = req_rate;
		const double samp_rate_fract = req_rate - (double) samp_rate_uint;
		//setup streaming
		usrp->set_time_now(uhd::time_spec_t(0.0));
		rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

		uint64_t last_firstsample = 0;

		while(is_running){
			// if next call will overflow our buffer, call the callback
			if (items_per_api_read + items_in_buffer > buff_size) {
				int64_t dropped_samples = 0;
				const uint64_t samples_in_buffer = items_in_buffer >> 1;

				// estimate dropped samples
				if (md.has_time_spec) {
					const uint64_t roundsecs = (uint64_t) md.time_spec.get_full_secs();
					uint64_t first_sample_id = roundsecs * samp_rate_uint;
					first_sample_id += roundsecs * samp_rate_fract + 0.5;
					first_sample_id += md.time_spec.get_frac_secs() * req_rate + 0.5;

					// we should have the id of the first sample in our first_sample_id
					const uint64_t expected_first_sample_id = last_firstsample + samples_in_buffer;
					const int64_t dropped_samples_now = first_sample_id - expected_first_sample_id;

					dropped_samples = dropped_samples_now;

					// estimate next frame
					last_firstsample = first_sample_id;
				}

				if (dropped_samples <= 0)
					cb(buff, items_in_buffer, ctx, 0); // nothing was dropped, nice
				else if ((dropped_samples / ((float) samples_in_buffer)) < FRACT_DROPPED_TO_TOLERATE)
					cb(buff, items_in_buffer, ctx, dropped_samples); // some part of the data was dropped, but that's fine
				else
					cb(buff, 0, ctx, dropped_samples + samples_in_buffer); // too much dropped, abort

				// reset counters, the native buffer is empty
				items_in_buffer = 0;
			}

			size_t num_rx_samps = rx_stream->recv(
					&buff[items_in_buffer], samples_per_api_read, md
			);

			// vizualize gap
			items_in_buffer+=(num_rx_samps << 1);

			//handle the error codes
			switch(md.error_code){
			case uhd::rx_metadata_t::ERROR_CODE_NONE:
				break;

			case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
				std::cout << boost::format(
						"Got timeout before all samples received, possible packet loss."
				) << std::endl;
				break;
			case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
				//printf("Overflow!\n"); fflush(stdout);
				break;

			default:
				std::cout << boost::format(
						"Got error code 0x%x, exiting loop..."
				) % md.error_code << std::endl;
				goto done_loop;
			}

		} done_loop:

		usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

	  // fix from https://github.com/j-helen/USRP-Receiver/blob/e4d845f1bfc5f0cafa1e89d612cf6b2714d46ca0/usrp_src.cpp  
    while(rx_stream->recv(
	        buff, samples_per_api_read, md,
	        0.1,true
	    )){
	        /* NOP */
	    };
	}
	catch (std::exception const&  ex)
	{
    std::cout << "tsdr_plugin tag2" << std::endl;
		is_running = 0;
		if (buff!=NULL) free(buff);
		RETURN_EXCEPTION(ex.what(), TSDR_CANNOT_OPEN_DEVICE);
	}
	if (buff!=NULL) free(buff);
	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API void __stdcall tsdrplugin_cleanup(void) {

	try {
		usrp.reset();
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	} catch (std::exception const&  ex) {
    std::cout << "tsdrplugin_catch3" << std::endl;

	}

	is_running = 0;
}
