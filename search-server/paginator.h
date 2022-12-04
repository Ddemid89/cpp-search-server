#pragma once

#include <ostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange (Iterator range_begin, Iterator range_end, size_t size) : range_begin_(range_begin),
                                                                            range_end_(range_end),
                                                                            size_(size) {}
    Iterator begin() const {
        return range_begin_;
    }
    Iterator end() const {
        return range_end_;
    }
    size_t size() {
        return size_;
    }
private:
    Iterator range_begin_;
    Iterator range_end_;
    size_t size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (const auto It : range) {
        out << It;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator (Iterator range_begin, Iterator range_end, size_t page_size) {
        Iterator page_begin = range_begin;
        Iterator page_end = page_begin;
        while (page_end != range_end){
            if (distance(page_begin, range_end) >= page_size) {
                page_end = page_begin;
                advance(page_end, page_size);
            } else {
                page_end = range_end;
            }
            pages_.push_back(IteratorRange<Iterator>(page_begin, page_end, page_size));
            page_begin = page_end;
        }
    }
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& container, size_t page_size) {
    return Paginator(begin(container), end(container), page_size);
}