//
// --------  Замер работы блока кода ----------
//

#ifndef SEARCH_SERVER_LOG_DURATION_H
#define SEARCH_SERVER_LOG_DURATION_H


#include <chrono>
#include <iostream>
#include <utility>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define CODE_DURATION(x) CodeDuration UNIQUE_VAR_NAME_PROFILE(x)
#define CODE_DURATION_STREAM(x, s) CodeDuration UNIQUE_VAR_NAME_PROFILE(x, s)
#define SEARCH_SERVER_DURATION SearchServerDuration UNIQUE_VAR_NAME_PROFILE
#define SEARCH_SERVER_DURATION_STREAM(s) SearchServerDuration UNIQUE_VAR_NAME_PROFILE(s)

class LogDuration {
protected:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    explicit LogDuration(std::ostream &stream) : stream_(stream) {}


    std::ostream &stream_;
};

class CodeDuration : LogDuration {
public:

    explicit CodeDuration(std::string_view id, std::ostream &stream = std::cerr) : LogDuration(stream), id_(std::move(id)) {
    }

    ~CodeDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;

        stream_ << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
};

class SearchServerDuration : LogDuration {
public:

    explicit SearchServerDuration(std::ostream &stream = std::cout) : LogDuration(stream) {
    }

    ~SearchServerDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;

        stream_ << "Operation time: "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
};

#endif //SEARCH_SERVER_LOG_DURATION_H
