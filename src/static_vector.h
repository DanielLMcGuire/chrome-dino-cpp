#pragma once

template <class T, int N>
class StaticVector {
public:
    static constexpr int capacity() { return N; }

    [[nodiscard]] bool empty() const { return n_ == 0; }
    [[nodiscard]] bool full()  const { return n_ >= N; }
    [[nodiscard]] int  size()  const { return n_; }

    T&       operator[](int i)       { return items_[i]; }
    const T& operator[](int i) const { return items_[i]; }
    T&       back()                  { return items_[n_ - 1]; }
    const T& back() const            { return items_[n_ - 1]; }

    T*       begin()       { return items_; }
    T*       end()         { return items_ + n_; }
    const T* begin() const { return items_; }
    const T* end()   const { return items_ + n_; }

    void clear() { n_ = 0; }

    bool push_back(const T& v) {
        if (n_ >= N) return false;
        items_[n_++] = v;
        return true;
    }

    void pop_front() {
        if (n_ == 0) return;
        for (int i = 1; i < n_; ++i) items_[i - 1] = items_[i];
        --n_;
    }

    template <class Pred>
    void remove_if(Pred pred) {
        int w = 0;
        for (int r = 0; r < n_; ++r) {
            if (pred(items_[r])) continue;
            if (w != r) items_[w] = items_[r];
            ++w;
        }
        n_ = w;
    }

private:
    T   items_[N];
    int n_ = 0;
};
