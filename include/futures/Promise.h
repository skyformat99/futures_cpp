#pragma once
#include <futures/Future.h>
#include <futures/channel/OneShotChannel.h>

namespace futures {

class PromiseException : public std::runtime_error {
public:
    PromiseException()
        : std::runtime_error("invalid promise state") {}
};

template <typename T, class Lock = std::mutex>
class PromiseFuture : public FutureBase<PromiseFuture<T, Lock>, T> {
public:
    Poll<T> poll() override {
        if (v_.hasException() || v_.hasValue())
            return makePollReady(std::move(v_));
        auto p = recv_.poll();
        if (p.hasException())
            return Poll<T>(p.exception());
        auto r = folly::moveFromTry(p);
        if (r.isReady()) {
            return makePollReady(std::move(r).value());
        } else {
            return Poll<T>(not_ready);
        }
    }

    explicit PromiseFuture(channel::OneshotChannelReceiver<Try<T>, Lock> &&recv)
        : recv_(std::move(recv)) {
    }

    explicit PromiseFuture(Try<T> &&recv)
        : v_(std::move(recv)) {
    }

    explicit PromiseFuture(const Try<T> &recv)
        : v_(recv) {
    }
private:
    channel::OneshotChannelReceiver<Try<T>, Lock> recv_;
    Try<T> v_;
};


template <typename T, typename Lock = std::mutex>
class Promise {
public:
    Promise() {
        auto p = channel::makeOneshotChannel<Try<T>, Lock>();
        s_ = std::move(p.first);
        r_ = std::move(p.second);
    }

    PromiseFuture<T> getFuture() {
        if (!r_.isValid()) throw PromiseException();
        return PromiseFuture<T, Lock>(std::move(r_));
    }

    void cancel() {
        s_.cancel();
    }

    template <typename V>
    bool setValue(V&& v) {
        return s_.send(Try<T>(std::forward<V>(v)));
    }

    void setException(folly::exception_wrapper ex) {
        s_.send(Try<T>(ex));
    }

private:
    channel::OneshotChannelSender<Try<T>, Lock> s_;
    channel::OneshotChannelReceiver<Try<T>, Lock> r_;
};

template <typename T, typename Lock = std::mutex,
         typename T0 = typename T::element_type>
PromiseFuture<T0> makePromiseFuture(T&& v) {
    return PromiseFuture<T0, Lock>(std::forward<T>(v));
}

template <typename T, typename Lock = std::mutex,
         typename T0 = typename std::remove_reference<T>::type>
PromiseFuture<T0> makeReadyPromiseFuture(T&& v) {
    return PromiseFuture<T0, Lock>(Try<T0>(std::forward<T>(v)));
}

}
