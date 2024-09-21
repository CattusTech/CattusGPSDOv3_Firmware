#pragma once
#include <exception>
#include <stm32g4xx_hal.h>
namespace periph
{
    class hardware_error : std::exception
    {
    private:
        const char* what_str = nullptr;
        const char* code_str = nullptr;

    public:
        hardware_error(HAL_StatusTypeDef status, const char* str) noexcept;
        virtual const char* what() const noexcept override;
        virtual const char* code() const noexcept;
        ~hardware_error() = default;
    };
} // namespace periph
