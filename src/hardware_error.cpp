#include "hardware_error.hpp"
using namespace periph;
hardware_error::hardware_error(HAL_StatusTypeDef status, const char* str) noexcept
{
    switch (status)
    {
        case HAL_OK:
            this->code_str = "no error";
            break;
        case HAL_ERROR:
            this->code_str = "failed";
            break;
        case HAL_BUSY:
            this->code_str = "busy";
            break;
        case HAL_TIMEOUT:
            this->code_str = "time out";
            break;
        default:
            this->code_str = "unknown error";
            break;
    }
    this->what_str = str;
}
const char* hardware_error::what() const noexcept
{
    return this->what_str;
}
const char* hardware_error::code() const noexcept
{
    return this->code_str;
}
