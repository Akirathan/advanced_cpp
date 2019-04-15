#include <exception>
#include <string>

#ifndef ALLOCATOR_EXCEPTION_HPP
#define ALLOCATOR_EXCEPTION_HPP

class AllocatorException : public std::exception {
public:
    explicit AllocatorException(const std::string &msg)
        : msg{msg}
    {}

    virtual const char *what() const throw()
    {
        return msg.c_str();
    }

private:
    std::string msg;
};

#endif //ALLOCATOR_EXCEPTION_HPP
