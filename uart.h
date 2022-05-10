//=========================================================================================================
// uart.h - Defines a mechanism for interfacing to a UART
//=========================================================================================================
#pragma once
#include <stdint.h>
#include <hls_stream.h>

class CUART
{
public:

    // Similar to printf()
    void    print(const char* fmt, uint32_t v0 = 0, uint32_t v1 = 0, uint32_t v2 = 0, uint32_t v3 = 0);

    // Writes a number to the output in ASCII decimal
    void    write_dec(uint32_t value, bool is_signed, uint8_t width);

    // Writes a number to the output in ASCII hex
    void    write_hex(uint32_t value, bool zero_fill, bool uppercase,  uint8_t width);

    // These write a single 8-bit value to the _xmit_fifo
    void    write_char(const char    c);
    void    write_byte(const uint8_t c);

    // Call this to fetch a byte from the UART
    bool    read_byte(uint8_t* c, bool blocking=true);

protected:

    // This FIFO connects to a fifo_to_uart module
    hls::stream<uint8_t> _xmit_fifo;
    hls::stream<uint8_t> _recv_fifo;


};


