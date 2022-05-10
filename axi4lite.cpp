//=========================================================================================================
// axi4lite.cpp - Implements a class that communicates with the fifo_to_axi4lite.v module in order to 
//                perform AXI4-Lite read/write transactions
//=========================================================================================================
#include "axi4lite.h"



//=========================================================================================================
// write() - Writes the specified value to the specified AXI register
//
// Returns: The AXI BRESP signal.  Non-zero = error.
//=========================================================================================================
uint8_t Caxi4lite::write(const uint32_t addr, const uint32_t data)
{
    const int65 rw_bit = ((int65)1) << 64;
    int65 value = rw_bit | ((int65)data) << 32 | addr;
    int34 response = 0;

    for (int i=0; i<2; ++i)
    {
        #pragma HLS pipeline off
        switch(i)
        {
        case 0:	_cmd_fifo.write(value);
                break;
        case 1: response = _rsp_fifo.read();
                break;
        }
    }

    return (uint8_t)(response >> 32);
}
//=========================================================================================================



//=========================================================================================================
// read() - Reads the AXI register at the specified address.
//
// On Exit: *p_data = The 32-bit value of the AXI register that was read
//
// Returns: The AXI RRESP signal.   Non-zero = error.
//=========================================================================================================
uint8_t Caxi4lite::read(const uint32_t addr, uint32_t* p_data)
{
    int34 response = 0;

    for (int i=0; i<2; ++i)
    {
        #pragma HLS pipeline off
        switch(i)
        {
        case 0:	_cmd_fifo.write(addr);
                break;
        case 1:	response = _rsp_fifo.read();
                *p_data = (uint32_t)(response & 0xFFFFFFFF);
                break;
        }
    }

    return (uint8_t)(response >> 32);
}
//=========================================================================================================
