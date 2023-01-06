//
// --------  Управление потоками ввода/вывода ----------
//

#ifndef SEARCH_SERVER_STREAM_UNITER_H
#define SEARCH_SERVER_STREAM_UNITER_H

#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define STREAM_UNITER(x) StreamUntier UNIQUE_VAR_NAME_PROFILE(x)

class StreamUntier {
public:
    explicit StreamUntier(const std::istream &stream = std::cin) : stream_(const_cast<std::istream &>(stream)) {
        tied_before_ = stream_.tie(nullptr);
    }

    ~StreamUntier() {
        stream_.tie(tied_before_);
    }

private:
    std::ostream *tied_before_;
    std::istream &stream_;
};

#endif //SEARCH_SERVER_STREAM_UNITER_H
