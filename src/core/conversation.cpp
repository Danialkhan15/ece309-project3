#include "core/conversation.h"

#include <stdexcept>
#include <utility>
//Constructor
Conversation::Conversation() = default;
//Destructor
Conversation::~Conversation() {
    delete[] data_;
}
//Copy constructor
Conversation::Conversation(const Conversation& other)
    : data_(nullptr), size_(0), capacity_(0) {
    if (other.capacity_ == 0) return;

    Message* copied = new Message[other.capacity_];
    try {
        for (std::size_t i = 0; i < other.size_; ++i) {
            copied[i] = other.data_[i];
        }
    } catch (...) {
        delete[] copied;
        throw;
    }

    data_ = copied;
    size_ = other.size_;
    capacity_ = other.capacity_;
}
//Copy assignment operator
Conversation& Conversation::operator=(const Conversation& other) {
    if (this == &other) return *this;

    Message* copied = nullptr;
    if (other.capacity_ != 0) {
        copied = new Message[other.capacity_];
        try {
            for (std::size_t i = 0; i < other.size_; ++i) {
                copied[i] = other.data_[i];
            }
        } catch (...) {
            delete[] copied;
            throw;
        }
    }

    delete[] data_;
    data_ = copied;
    size_ = other.size_;
    capacity_ = other.capacity_;
    return *this;
}

//Move constructor
Conversation::Conversation(Conversation&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}
//Move assignment operator
Conversation& Conversation::operator=(Conversation&& other) noexcept {
    if (this == &other) return *this;

    delete[] data_;
    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;

    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
    return *this;
}
//Append
void Conversation::append(Message m) {
    if (size_ == capacity_) {
        const std::size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
        Message* grown = new Message[new_capacity];
        try {
            for (std::size_t i = 0; i < size_; ++i) {
                grown[i] = data_[i];
            }
            grown[size_] = std::move(m);
        } catch (...) {
            delete[] grown;
            throw;
        }

        delete[] data_;
        data_ = grown;
        capacity_ = new_capacity;
    } else {
        data_[size_] = std::move(m);
    }

    ++size_;
}
//Size
std::size_t Conversation::size() const noexcept {
    return size_;
}
//Finding a message at a specific index with bounds checking
const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) {
        throw std::out_of_range("Conversation index out of range");
    }
    return data_[i];
}
//Where do messages begin
const Message* Conversation::begin() const noexcept {
    return data_;
}
//Where do messages end
const Message* Conversation::end() const noexcept {
    return data_ == nullptr ? nullptr : data_ + size_;
}
