//=========================================================================================================
// hls_clock.h - Defines a mechanism for communicating with a "hls_clock.v" module
//=========================================================================================================
#pragma once
#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>

class CHLSClock
{
public:

    // Call this to fetch the current time in microseconds
    uint64_t    time();

    // Call this to reset the clock back to zero
    void        reset();

protected:

    // This FIFO connects to a fifo_to_uart module
    hls::stream< ap_uint<1> > _cmd_fifo;
    hls::stream< uint64_t   > _rsp_fifo;

};


