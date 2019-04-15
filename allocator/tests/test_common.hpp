#ifndef TEST_COMMON_HPP
#define TEST_COMMON_HPP

#include <sys/time.h>
#include <functional>


inline double get_wall_time()
{
    struct timeval time;
    if (gettimeofday(&time, nullptr)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

inline double get_cpu_time()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

inline double count_slowdown(double std_time, double my_time)
{
    return my_time / std_time;
}

inline double measure(std::function<void(void)> func)
{
    double wall_time_before = get_wall_time();
    func();
    double wall_time_after = get_wall_time();
    return wall_time_after - wall_time_before;
}

#endif //TEST_COMMON_HPP
