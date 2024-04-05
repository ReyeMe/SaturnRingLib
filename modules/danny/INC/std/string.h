#pragma once

#include "utils.h"

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

namespace std
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
    };
}
