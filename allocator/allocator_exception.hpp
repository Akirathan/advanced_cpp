#include <exception>

#ifndef UNIT_TESTS_ALLOCATOR_EXCEPTION_HPP
#define UNIT_TESTS_ALLOCATOR_EXCEPTION_HPP

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

#endif //UNIT_TESTS_ALLOCATOR_EXCEPTION_HPP
