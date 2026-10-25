#pragma once
#include <type_traits>
#include <stdarg.h>
#include <cstdint>
extern "C" {
    int snprintf(char* buffer, size_t n, const char* fmt, ...);
    size_t	strlen(const char* str);
    char* strcpy(char* dst, const char* src);
    char* strncpy(char* dst, const char* src, size_t len);
    char* strcat(char* dst, const char* src);
    char* strncat(char* dst, const char* src, size_t len);
    int	strcmp(const char* s1, const char* s2);
    int	strncmp(const char* s1, const char* s2, size_t len);
    char* strchr(const char* str, int ch);
    char* strrchr(const char*, int);
    char* strpbrk(const char* str, const char* set);
    size_t	strspn(const char* str, const char* set);
    size_t	strcspn(const char* str, const char* set);
    char* strstr(const char* str, const char* sub);
    int	memcmp(const void* p1, const void* p2, size_t len);
    void* memcpy(void* dst, const void* src, size_t len);
    void* memmove(void* dst, const void* src, size_t len);
    void* memset(void* dst, int ptn, size_t len);
    void* memchr(const void* mem, int ptn, size_t len);
}

#if !DOXYGEN

namespace SRL
{
    class string
    {
    private:
        char* str = nullptr;

    public:
        // Default constructor
        string() : str(nullptr) {}

        // Constructor with source string
        string(const char* src)
        {
            str = new char[strlen(src) + 1];
            strcpy(str, src);
        }

        template <typename... Args>
        string(const char* format, Args... args)
        {
            size_t size = snprintf(nullptr, 0, format, args...) + 1;
            str = new char[size];
            snprintf(str, size, format, args...);
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        string(const T& integer)
        {
            size_t size = snprintf(nullptr, 0, "%d", integer) + 1;
            str = new char[size];
            snprintf(str, size, "%d", integer);
        }

        // Copy constructor
        string(const string& other)
        {
            str = new char[strlen(other.str) + 1];
            strcpy(str, other.str);
        }

        // Copy assignment operator
        string& operator=(const string& other)
        {
            if (this != &other)
            {
                delete[] str;
                str = new char[strlen(other.str) + 1];
                strcpy(str, other.str);
            }
            return *this;
        }

        // Move constructor
        string(string&& other) noexcept
            : str(other.str)
        {
            other.str = nullptr;
        }

        // Move assignment operator
        string& operator=(string&& other) noexcept
        {
            if (this != &other)
            {
                delete[] str;
                str = other.str;
                other.str = nullptr;
            }
            return *this;
        }

        // Overloaded + operator for string concatenation
        string operator+(const string& other) const
        {
            string result;
            result.str = new char[strlen(str) + strlen(other.str) + 1];
            strcpy(result.str, str);
            strcat(result.str, other.str);
            return result;
        }

        // Destructor
        ~string()
        {
            delete[] str;
        }


        const char* c_str() const
        {
            return str;
        }

         /**
         * @brief Extension of snprintf with support of SRL::Math::Types::Fxp. Supported format specifiers %c , %s , %u , %d, %x and %f. %f is used for FXP types. %0Nd where N is the total lenght of the printed number. If under N , the number will be padded with 0's until the printed string has N characters
         * @param buffer Buffer where the string will be written to
         * @param size maximum size of string to be written into buffer
         * @param format format string
         * 
         */
        int snprintfEx(char* buffer, const int size, const char* format, ... )
         { 
            va_list args;
            va_start(args, format);
            int writtenChars = 0;
            for(int idx = 0 ; format[idx] != '\0'; idx++)
            {
                //go through all the format string
                //check if a format delimiter was found. avoid instances of %%
                if(format[idx] == '%' && format[idx+1] != '%' && format[idx+1] != '\0')
                {
                    switch(format[idx+1])
                    {
                        case 'c' : //char
                            
                            if(writtenChars < size)
                            {
                                buffer[writtenChars] = va_arg(args, int);
                                writtenChars++;
                            }else
                            {
                                writtenChars++;
                            }
        
                        break;
                        case 's' : //string
                            {
                                char* arg = va_arg(args, char*);
                                for(int jdx = 0; arg[jdx] != 0  ; jdx++ , writtenChars++)
                                {
                                    if(writtenChars < size)
                                    {
                                        buffer[writtenChars] = arg[jdx];
                                    }
                                }
                            }
                        break;
                        case '0' : // only %0d is supported for now
                            {
                                char tmp[100] = {0};
                                int arg = va_arg(args, int);
                                //print integer into to get its lenght
                                int int_lenght = snprintf(tmp, 100, "%d", arg);
                                //get the padding requested
                                if(format[idx+2] != '\0')
                                {
                                    int padding_req = format[idx+2] - '0';
                                    int padding = padding_req - int_lenght;
                                    
                                    if(padding > 0)
                                    {
                                        for(int i = 0 ; i < padding ; i++, writtenChars++)
                                        {
                                             if(writtenChars < size)
                                            {
                                                buffer[writtenChars] = '0';
                                            }
                                        }
                                    }
                                   
                                    for(int jdx = 0; tmp[jdx] != 0 ; jdx++ , writtenChars++)
                                    {
                                        if(writtenChars < size)
                                        {
                                            buffer[writtenChars] = tmp[jdx];
                                        }
                                        
                                    }
                                    
                                }
                                
                            idx = idx + 2;


                            }
                        break;

                        case 'd' : //signed int
                            {
                                char tmp[100] = {0};
                                int arg = va_arg(args, int);
                                snprintf(tmp, 100, "%d", arg);
                                for(int jdx = 0; tmp[jdx] != 0 ; jdx++ , writtenChars++)
                                {
                                    if(writtenChars < size)
                                    {
                                        buffer[writtenChars] = tmp[jdx];
                                    }
                                    
                                }
                            }
                        break;
                        case 'x' : //hexadecimal
                            {
                                char tmp[100] = {0};
                                int arg = va_arg(args, int);
                                snprintf(tmp, 100, "%x", arg);
                                for(int jdx = 0; tmp[jdx] != 0 ; jdx++ , writtenChars++)
                                {
                                    if(writtenChars < size)
                                    {
                                        buffer[writtenChars] = tmp[jdx];
                                    }
                                    
                                }
                            }
                        break;
                        case 'u' : //unsigned int
                            {
                                char tmp[100] = {0};
                                unsigned int arg = va_arg(args, unsigned int);
                                snprintf(tmp, 100, "%u", arg);
                                for(int jdx = 0; tmp[jdx] != 0 ; jdx++ , writtenChars++)
                                {
                                    if(writtenChars < size)
                                    {
                                        buffer[writtenChars] = tmp[jdx];
                                    }
                                }
                            }
                        break;
                        case 'f' : // FXP 16.16 fixed point
                        {
                            
                            char tmpBuffer[14] = {0};
                            int bufferPos = 0;
                            
                            SRL::Math::Types::Fxp *arg = va_arg(args,  SRL::Math::Types::Fxp*);
                            
                            int32_t val = arg->RawValue();
                            
                            //determine if value is negative
                            bool isNegative = val < 0;
        
                            uint32_t absValue = static_cast<uint32_t>(isNegative ? -val : val);
                            int32_t IntegerPart = absValue  >> 16;
                                                             
                            // Scale fractional part to 5 decimal digits
                            int32_t FractionPart = SRL::Math::Abs((arg->GetFraction() * 1.5258789).RawValue());
                            
                            if(isNegative)
                            {
                                tmpBuffer[bufferPos++] = '-';
                            }
        
                            // Convert integer part to decimal (in reverse)
                            char intBuffer[6];
                            int intLength = 0;
        
                            if (IntegerPart == 0) {
                                intBuffer[intLength++] = '0';
                            } else {
                                while (IntegerPart > 0 && intLength < 6) {
                                    intBuffer[intLength++] = '0' + (IntegerPart % 10);
                                    IntegerPart /= 10;
                                }
                            }
        
                            // Write integer part in correct order
                            for (int idx = intLength - 1; idx >= 0 && bufferPos < 13; --idx) {
                                tmpBuffer[bufferPos++] = intBuffer[idx];
                                }
        
                            // Decimal point
                            if (bufferPos < 13) {
                                tmpBuffer[bufferPos++] = '.';
                             }
        
                            // Convert fractional part to decimal with leading zeros
                            uint32_t divisor = 10000;
                            for (int idx = 0; idx < 5 && bufferPos < 13; ++idx) {
                                tmpBuffer[bufferPos++] = '0' + (FractionPart / divisor);
                                FractionPart %= divisor;
                                divisor /= 10;
                            }
        
                            // Null terminator
                            tmpBuffer[bufferPos] = '\0';
                            char tmp[100] = {0};
        
                            snprintf(tmp, 100 ,"%s", tmpBuffer);
                            for(int j = 0; tmp[j] != 0 ; j++ , writtenChars++)
                            {
                                if(writtenChars < size)
                                {
                                    buffer[writtenChars] = tmp[j];
                                } 
                            }
        
                        }
                        break;
                        default : //nothing
                        break;
        
                    }
        
                    idx++;
                }
                else
                {
                    if(writtenChars < size )
                    {
                        buffer[writtenChars] = format[idx];
                        writtenChars++;
                    }else
                    {
                        writtenChars++; 
                    }
                }
            }
            buffer[writtenChars] = '\0';
            va_end(args);
            return writtenChars;
        }
    };
}

#endif

namespace SRL
{
    /**
     * @brief Converts a hex character to its integer value.
     * @param ch Character to convert ('0'-'9', 'a'-'f' or 'A'-'F')
     * @return Value 0-15, or -1 if ch is not a hex digit
     */
    inline int hex(char ch)
    {
        if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F')
            return ch - 'A' + 10;
        if (ch >= '0' && ch <= '9')
            return ch - '0';
        return -1;
    }

    /**
     * @brief Converts a 4-bit integer to its hex character equivalent.
     * @param v Value to convert (only the low 4 bits are used)
     * @return Lowercase hex digit character
     */
    inline char hexchar(int v)
    {
        v &= 0xf;
        return v < 10 ? '0' + v : 'a' + v - 10;
    }

    /**
     * @brief Decodes a hex string into memory, using 32-/16-bit stores where
     * address and remaining length allow, falling back to byte stores only
     * for the unaligned remainder.
     * @details VDP RAM (e.g. VDP2 CRAM) drops single-byte writes: a byte-wise 'M' of
     * 0xEC63 reads back as 0x0063. Whole aligned words/halfwords match its bus width.
     * @param buf Hex string to decode (2 characters per byte)
     * @param addr Destination address
     * @param count Number of bytes to decode
     * @return Pointer to the character following the decoded hex string, or nullptr on failure.
     */
    inline const char *hex2mem_aligned(const char *buf, uint32_t addr, int count)
    {
        // Validate all characters first to prevent partial memory corruption
        for (int i = 0; i < count * 2; i++)
        {
            if (hex(buf[i]) < 0)
                return nullptr;
        }

        int i = 0;
        while (i < count)
        {
            const int remaining = count - i;
            const uint32_t cur = addr + static_cast<uint32_t>(i);
            if (remaining >= 4 && (cur & 3U) == 0U)
            {
                uint32_t v = 0;
                for (int b = 0; b < 4; b++)
                {
                    const int h1 = hex(*buf++);
                    const int h2 = hex(*buf++);
                    v = (v << 8) | static_cast<uint32_t>((h1 << 4) | h2);
                }
                *reinterpret_cast<volatile uint32_t *>(cur) = v;
                i += 4;
            }
            else if (remaining >= 2 && (cur & 1U) == 0U)
            {
                uint16_t v = 0;
                for (int b = 0; b < 2; b++)
                {
                    const int h1 = hex(*buf++);
                    const int h2 = hex(*buf++);
                    v = static_cast<uint16_t>((v << 8) | static_cast<uint16_t>((h1 << 4) | h2));
                }
                *reinterpret_cast<volatile uint16_t *>(cur) = v;
                i += 2;
            }
            else
            {
                const int h1 = hex(*buf++);
                const int h2 = hex(*buf++);
                *reinterpret_cast<volatile uint8_t *>(cur) = static_cast<uint8_t>((h1 << 4) | h2);
                i += 1;
            }
        }
        return buf;
    }
}