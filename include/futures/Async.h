#pragma once

#include <futures/core/Try.h>
#include <futures/core/Optional.h>

namespace futures {

using folly::Try;
using folly::Optional;

class AsyncNotReadyException : public std::runtime_error {
 public:
  AsyncNotReadyException()
      : std::runtime_error("Not ready Async cannot be unwrapped") {}
};


template <typename T>
class Async {
    enum class State {
        Ready,
        NotReady,
    };

public:
    typedef T item_type;

    Async() : state_(State::NotReady) {}
    ~Async() {}

    explicit Async(const T& v) : state_(State::Ready), v_(v) {}
    explicit Async(T&& v) : state_(State::Ready), v_(std::move(v)) {}

    // Move constructor
    Async(Async<T>&& t) noexcept
        : state_(t.state_) {
        if (state_ == State::Ready)
            new (&v_) T(std::move(t.v_));
    }

    // Move assigner
    Async& operator=(Async<T>&& t) noexcept {
        if (this == &t)
            return *this;
        this->~Async();
        state_ = t.state_;
        if (state_ == State::Ready)
            new (&v_) T(std::move(t.v_));
        return *this;
    }

    // Copy constructor
    Async(const Async& t) {
        static_assert(
                std::is_copy_constructible<T>::value,
                "T must be copyable for Try<T> to be copyable");
        state_ = t.state_;
        if (state_ == State::Ready)
            new (&v_) T(t.v_);
    }

    // Copy assigner
    Async& operator=(const Async& t) {
        static_assert(
                std::is_copy_constructible<T>::value,
                "T must be copyable for Try<T> to be copyable");
        if (this == &t)
            return *this;
        this->~Async();
        state_ = t.state_;
        if (state_ == State::Ready)
            new (&v_) T(t.v_);
        return *this;
    }

    bool operator==(const Async& t) const noexcept {
        if (state_ == t.state_) {
            if (state_ == State::NotReady)
                return true;
            return v_ == t.v_;
        } else {
            return false;
        }
    }

    bool isReady() const {
        return state_ == State::Ready;
    }

    bool isNotReady() const {
        return state_ == State::NotReady;
    }

    template <typename F>
    Async<typename std::result_of<F(T)>::type> map(F&& fn) {
        typedef typename std::result_of<F(T)>::type ret_type;
        if (state_ == State::Ready)
            return Async<ret_type>(fn(v_));
        return Async<ret_type>();
    }

    const T& value() const& {
        require_ready();
        return v_;
    }

    T& value() & {
        require_ready();
        return v_;
    }

    T value() && {
        require_ready();
        return std::move(v_);
    }

private:
    void require_ready() const {
        if (state_ != State::Ready)
            throw AsyncNotReadyException();
    }

    State state_;
    T v_;
};

}
