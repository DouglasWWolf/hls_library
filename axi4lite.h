//=========================================================================================================
// axi4lite.h - Defines a class that communicates with the fifo_to_axi4lite.v module in order to 
//              perform AXI4-Lite read/write transactions
//=========================================================================================================
#pragma once
#include <hls_stream.h>
#include <ap_int.h>
#include <stdint.h>

class Caxi4lite
{
public:

    // AXI write transaction - Writes the specified data to the specified address
    // Returns non-zero on error (returns the value from the AXI bresp channel)
    uint8_t write(const uint32_t addr, const uint32_t data);
    
    // AXI read transaction - Reads the specified address.   Returns non-zero on
    // error. (returns the value of the AXI rresp channel)
    uint8_t read(const uint32_t addr, uint32_t* p_data);

private:
    
    typedef ap_uint<65> int65;
    typedef ap_uint<34> int34;

    hls::stream< int65 > _cmd_fifo;
    hls::stream< int34 > _rsp_fifo;
};
