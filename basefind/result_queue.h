/* Simple class that allows the programmer to store the best / worst
 * n results of an operator. The programmer has to supply
 * a comparator for his data. If they want the best n result
 * of an operator, std::greater is needed. Otherwise, std::less
 * is what you want to use
 */

#ifndef _RESULT_QUEUE_H
#define _RESULT_QUEUE_H

#include <queue>
#include <stack>
#include <algorithm>
#include <iterator>

template <typename T, typename Comp>
class result_queue
{
public:
    result_queue(std::size_t capacity) : capacity{capacity}
    {}

    void addResult(const T &result)
    {
        q.push(result);
        if (q.size() > capacity) {
            q.pop();
        }
    }

    /* Prints out the queue, by using a user supplied
       function. If the underlying queue uses std::greater,
       the output is reversed and the 'best' result appears last. */
    template <typename Func>
    void printResults(Func print)
    {
        while (q.size() > 0) {
            std::cout << "[Result " << std::dec << capacity-- << "]: ";
            print(q.top());
            q.pop();
        }
    }
private:
    std::priority_queue<T, std::vector<T>, Comp> q;
    std::size_t capacity;
};

#endif // _RESULT_QUEUE_H