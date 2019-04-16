#include <exception>
#include <string>

#ifndef ALLOCATOR_EXCEPTION_HPP
#define ALLOCATOR_EXCEPTION_HPP

class allocator_exception : public std::exception {
public:
    explicit allocator_exception(const std::string &msg)
        : msg{msg}
    {}

    const char *what() const noexcept override
    {
        return msg.c_str();
    }

private:
    std::string msg;
};

#endif //ALLOCATOR_EXCEPTION_HPP
