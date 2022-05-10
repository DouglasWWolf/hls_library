//=========================================================================================================
// uart.cpp - Implements a mechanism for interfacing to a UART
//=========================================================================================================
#include "uart.h"


//=========================================================================================================
// print() - Analog of C function printf().  Supports %d, %i, %u, %c, %x %X
//=========================================================================================================
void CUART::print(const char fmt[128], uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3)
{
    const uint32_t v[] = { v0, v1, v2, v3 };
    uint8_t i, value_index = 0;
    
    // Loop through each character of the input string...
    for (uint8_t input_count = 0; input_count < 128; ++input_count)
    {
        // Pipelining this loop will result in timing violations
        #pragma HLS pipeline off

        // Fetch the next input character
        char c = *fmt++;

        // If we've hit the end of the format string, we're done
        if (c == 0) break;

        if (c == '\n')
        {
            write_char('\r');
            write_char('\n');
            continue;
        } 

        // Else, if the format is something other than '%' output it literally
        else if (c != '%')
        {
            write_char(c);
            continue;
        }

        // Else, if the next character is '%', output a '%'
        else if (*fmt == '%')
        {
            ++fmt;
            write_char('%');
        }

        // Else, if the next character is end-of-string, output a '%'
        else if (*fmt == 0)
        {
            write_char('%');
            break;
        }
        
        // Else, output a binary value decoded into ASCII
        else
        {
            uint8_t width = 0;

            // Are we going to zero-fill a formatted numeric value?
            bool zero_fill = (*fmt == '0');

            // Find out how many characters should be output for the formatted value
            // (i.e, decode up to three ASCII digits into a number)
            for (i=0; i<3; ++i) 
            {
                if (*fmt < '0' || *fmt > '9') break;
                width = (width << 3) + (width << 1);  // width = width * 10
                width = width + *fmt - '0';
                ++fmt;
            }

            // If there are no characters after the width specifier, we're done
            if (*fmt == 0) break;

            // Fetch the C/C++ printf-style format specifier
            char format_specifier = *fmt++;

            // Output a value converted to ASCII
            switch (format_specifier)
            {
            case 'd':
            case 'i':
                write_dec(v[value_index++], true, width);
                break;

            case 'u':
                write_dec(v[value_index++], false, width);
                break;

            case 'x':
                write_hex(v[value_index++], zero_fill, false, width);
                break;

            case 'X': 
                write_hex(v[value_index++], zero_fill, true, width);
                break;

            case 'c':
                write_char(v[value_index++] & 0xFF);
                break;

            default:
                write_char(format_specifier);
                break;
            }

            // Ensure that "value_index" doesn't fall of the end of the array
            if (value_index > 3) value_index = 3;
        }
    }

}
//=========================================================================================================


//=========================================================================================================
// write_dec() - Transmits the ASCII decimal characters that correspond to an input value
//=========================================================================================================
void CUART::write_dec(uint32_t value, bool is_signed, uint8_t width)
{
    static const int BUFFER_WIDTH = 32;
    char    buffer[BUFFER_WIDTH];
    bool    is_negative = false;
    char    digit;
    uint8_t i;

    static const uint32_t powers[] =
    {
        1000000000,
         100000000,
          10000000,
           1000000,
            100000,
             10000,
              1000,
               100,
                10,
                 1,
    };


    // Figure out the maximum number of decimal digits that we will output
    const uint8_t MAX_DIGITS = sizeof(powers) / sizeof(powers[0]);

    // Ensure that the caller's requested width will fit in our buffer 
    if (width > BUFFER_WIDTH - 1) width = BUFFER_WIDTH - 1;

    // Fill the output buffer with either spaces or ASCII zeros
    for (i = 0; i < BUFFER_WIDTH - 1; ++i) 
    {
        #pragma HLS unroll
        buffer[i] = ' ';
    }

    // Ensure that there's a nul-byte at the end of the buffer
    buffer[BUFFER_WIDTH - 1] = 0;

    // If we're supposed to interpret the input value as signed and the value is negative,
    // make the value positive.
    if (is_signed && (value & 0x80000000))
    {
        value = ~value + 1;
        is_negative = true;
    }

    // Point to the place where we will start storing decoded ASCII digits
    uint8_t out_index = BUFFER_WIDTH - MAX_DIGITS - 1;

    // We have not yet output the most significant digit
    uint8_t msd_index = 0;

    // Loop through each power of 10...
    for (uint8_t pow_idx = 0; pow_idx < MAX_DIGITS; ++pow_idx)
    {
        // This is the current power of 10 that we are working with
        uint32_t power_of_ten = powers[pow_idx];

        // Find out if we're processing the rightmost digit
        bool is_rightmost_digit = (power_of_ten == 1);

        // Determine the ASCII digit that corresponds to this power of ten
        if (is_rightmost_digit)
            digit = '0' + value;
        else
        {
            digit = '0';
            for (i=0; i<9; ++i)
            {
                if (value < power_of_ten) break;
                ++digit;
                value -= power_of_ten;
            }
        }

        // If we haven't yet output the most significant digit...
        if (msd_index == 0)
        {
            // If this digit is 0 and we're not outputting the 1's place, we'll output a space
            if (digit == '0' && !is_rightmost_digit) digit = ' ';

            // Otherwise, we have just encountered the first significant digit
            else
            {
                msd_index = out_index;
                if (is_negative)
                {
                    --msd_index;
                    buffer[msd_index] = '-';
                }
            }
        }

        // Stuff the digit into the buffer
        buffer[out_index++] = digit;
    }

    // Compute how many characters are in our output string
    uint8_t string_length = BUFFER_WIDTH - 1 - msd_index;

    // Determine how many characters we are going to output
    if (width > string_length)
    {
        string_length = width;
        msd_index = BUFFER_WIDTH - 1 - string_length;
    }

    // And write the characters to the UART
    for (i=0; i<string_length; ++i)
    {
        char c = buffer[msd_index + i];
        write_char(c);        
    }
}
//=========================================================================================================



//=========================================================================================================
// write_hex() - Transmits the ASCII hex characters that correspond to an input value
//=========================================================================================================
void CUART::write_hex(uint32_t value, bool zero_fill, bool uppercase, uint8_t width)
{
    static const int BUFFER_WIDTH = 16;
    char    buffer[BUFFER_WIDTH];
    uint8_t i;

    // Number of bits to shift our value in order to obtain the desired nybble
    static const uint8_t shift[] = { 28, 24, 20, 16, 12, 8, 4, 0 };

    // Determine what character will fill non-significant digits
    const char fill_char = zero_fill ? '0' : ' ';

    // Figure out the maximum number of hexadecimal digits that we will output
    const uint8_t MAX_DIGITS = sizeof(shift);

    // Determine what to add to a nybble in order to obtain ascii "A thru F" or "a thru f"
    const uint8_t hex_offset = (uppercase) ? 55 : 87;

    // Ensure that the caller's requested width will fit in our buffer 
    if (width > BUFFER_WIDTH - 1) width = BUFFER_WIDTH - 1;

    // Fill the output buffer with either spaces or ASCII zeros
    for (i = 0; i < BUFFER_WIDTH - 1; ++i)
    {
        #pragma HLS unroll
        buffer[i] = fill_char;
    }

    // Ensure that there's a nul-byte at the end of the buffer
    buffer[BUFFER_WIDTH - 1] = 0;

    // Point to the place where we will start storing decoded ASCII digits
    uint8_t out_index = BUFFER_WIDTH - MAX_DIGITS - 1;

    // We have not yet output the most significant digit
    uint8_t msd_index = 0xFF;

    // Loop through each power of 10...
    for (int digit_index = 0; digit_index < MAX_DIGITS; ++digit_index)
    {
        // Fetch the nybble that corresponds to this digit
        uint8_t nybble = (value >> shift[digit_index]) & 0xF;

        // Translate that nybble into an ASCII hex digit
        char digit = (nybble < 10) ? nybble + 48 : nybble + hex_offset;

        // If we haven't yet output the most significant digit...
        if (msd_index == 0xFF)
        {
            // If this digit is 0 and we're not outputting the lowest digit, we'll output a filler character
            if (digit == '0' && digit_index != (MAX_DIGITS-1)) digit = fill_char;

            // Otherwise, we have just encountered the first significant digit
            else msd_index = out_index;
        }

        // Stuff the digit into the buffer
        buffer[out_index++] = digit;
    }

    // Compute how many characters are in our output string
    uint8_t string_length = BUFFER_WIDTH - 1 - msd_index;

    // Determine how many characters we are going to output
    if (width > string_length)
    {
        string_length = width;
        msd_index = BUFFER_WIDTH - 1 - string_length;
    }

    // And write the characters to the UART
    for (i = 0; i < string_length; ++i)
    {
        char c = buffer[msd_index + i];
        write_char(c);
    }
}
//=========================================================================================================


//=========================================================================================================
// write_char() / write_byte() - Writes a single ASCII character to the FIFO that feeds the UART
//=========================================================================================================
void CUART::write_char(const char c)
{
    write_byte((uint8_t)c);
}

void CUART::write_byte(const uint8_t c)
{
    _xmit_fifo.write(c);
}
//=========================================================================================================

//=========================================================================================================
// read_byte() - Fetches a byte from the UART input stream
//=========================================================================================================
bool CUART::read_byte(uint8_t* c, bool blocking)
{
    uint8_t value;

    // If this is a blocking call, wait for a byte to arrive from the UART
    if (blocking)
    {
        _recv_fifo.read(value);
        *c = value;
        return true;
    }

    // This isn't a blocking call.   If there is a byte in the FIFO, 
    // hand it to the caller and then him he has a byte
    if (_recv_fifo.read_nb(value))
    {
        *c = value;
        return true;
    }

    // This wasn't a blocking call, and there was no byte waiting in the FIFO
    return false;
}
//=========================================================================================================

