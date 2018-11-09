#include <chrono>
#include <iostream>
#include <vector>
#include <queue>

// 测试数据
class InputData {
 public:
  InputData(const std::size_t num_arrays,
            const std::size_t num_elements_per_array) {
    std::cout << "Initializing input data ...";
    for (std::size_t i = 0; i < num_arrays; ++i) {
      std::vector<int> array;
      array.reserve(num_elements_per_array);
      int value = i;
      for (std::size_t j = 0; j < num_elements_per_array; ++j) {
        array.emplace_back(value);
        value += num_arrays;
      }
      arrays_.emplace_back(std::move(array));
    }
    std::cout << " done\n";
    std::flush(std::cout);
  }

  const std::vector<std::vector<int>>& arrays() const {
    return arrays_;
  }

 private:
  std::vector<std::vector<int>> arrays_;
};

// 简易的Iterator
class ArrayIterator {
 public:
  ArrayIterator()
      : current_(nullptr),
        end_(nullptr) {}

  ArrayIterator(const std::vector<int> &array)
      : current_(array.data()),
        end_(current_ + array.size()) {}

  inline int value() const {
    return *current_;
  }

  inline void next() {
    ++current_;
  }

  inline bool valid() const {
    return current_ < end_;
  }

  inline bool operator<(const ArrayIterator &other) const {
    return *current_< *other.current_;
  }

 private:
  const int *current_;
  const int * const end_;
};

using ArrayIteratorPointer = ArrayIterator*;

template <bool greater>
struct ArrayIteratorPointerComparator {
  inline bool operator()(const ArrayIteratorPointer &lhs,
                         const ArrayIteratorPointer &rhs) const {
    return greater ^ (*lhs < *rhs);
  }
};


// 拼接数组，直接用std::sort
void TestStdSort(const std::vector<std::vector<int>> &arrays,
                 std::vector<int> *output) {
  int *dst = output->data();
  for (const auto &array : arrays) {
    std::memcpy(dst, array.data(), array.size() * sizeof(int));
    dst += array.size();
  }
  std::sort(output->data(), dst);
}


// 使用Priority Queue的写法
void TestPriorityQueue(const std::vector<std::vector<int>> &arrays,
                       std::vector<int> *output) {
  std::priority_queue<ArrayIteratorPointer,
                      std::vector<ArrayIteratorPointer>,
                      ArrayIteratorPointerComparator<true>> pq;

  const std::size_t num_arrays = arrays.size();
  std::vector<ArrayIterator> iterators;
  iterators.reserve(num_arrays);
  for (std::size_t i = 0; i < num_arrays; ++i) {
    iterators.emplace_back(arrays[i]);
    pq.emplace(&iterators.back());
  }

  int* dst = output->data();
  while (!pq.empty()) {
    ArrayIteratorPointer it = pq.top();
    *(dst++) = it->value();
    pq.pop();
    it->next();
    if (it->valid()) {
      pq.push(it);
    }
  }
}


// 自定义Heap的写法：
class MergeHeap {
 public:
  using ArrayIteratorPointer = ArrayIterator*;

  MergeHeap(std::vector<ArrayIterator> *iterators) {
    heap_.reserve(iterators->size());
    for (ArrayIterator &it : *iterators) {
      heap_.emplace_back(&it);
    }
    std::sort(heap_.begin(), heap_.end(),
              ArrayIteratorPointerComparator<false>());
  }

  inline bool empty() const {
    return heap_.empty();
  }

  inline int next() {
    const ArrayIteratorPointer &top = heap_.front();
    const int value = top->value();
    top->next();
    if (!top->valid()) {
      heap_.front() = heap_.back();
      heap_.pop_back();
    }
    if (heap_.size() > 1) {
      siftDown();
    }
    return value;
  }

 private:
  inline void siftDown() {
    const ArrayIteratorPointer node = heap_.front();
    const std::size_t n = heap_.size();
    const std::size_t b = n >> 1;
    std::size_t k = 0;
    while (k < b) {
      const std::size_t l = (k << 1) + 1;
      const ArrayIteratorPointer lnode = heap_[l];

      const std::size_t r = l + 1;
      ArrayIteratorPointer rnode;
      if (r >= n || *lnode < *(rnode = heap_[r])) {
        if (*lnode < *node) {
          heap_[k] = lnode;
          k = l;
        } else {
          break;
        }
      } else {
        if (*rnode < *node) {
          heap_[k] = rnode;
          k = r;
        } else {
          break;
        }
      }
    }
    heap_[k] = node;
  }

  std::vector<ArrayIteratorPointer> heap_;
};

void TestCustomizedHeap(const std::vector<std::vector<int>> &arrays,
                        std::vector<int> *output) {
  const std::size_t num_arrays = arrays.size();
  std::vector<ArrayIterator> iterators;
  iterators.reserve(num_arrays);
  for (std::size_t i = 0; i < num_arrays; ++i) {
    iterators.emplace_back(arrays[i]);
  }

  MergeHeap heap(&iterators);
  int* dst = output->data();
  while (!heap.empty()) {
    *(dst++) = heap.next();
  }
}


void ValidateOutput(const std::vector<int> &output) {
  std::cout << "Validating output ... ";
  for (std::size_t i = 0; i < output.size(); ++i) {
    if (output[i] != i) {
      std::cout << "FAIL: " << "output[" << i << "] == " << output[i] << "\n";
      return;
    }
  }
  std::cout << "PASS\n";
}

int main(int argc, char *argv[]) {
  const std::size_t num_arrays = 1024;
  const std::size_t num_elements_per_array = 65536;
  const std::size_t total_num_elements = num_arrays * num_elements_per_array;

  InputData data(num_arrays, num_elements_per_array);
  std::vector<int> output(total_num_elements);

  using clock = std::chrono::high_resolution_clock;
  clock::time_point start_time, end_time;

  std::cout << "----\nTesting with prioriry queue ...\n";
  std::memset(output.data(), 0, sizeof(int) * total_num_elements);
  start_time = clock::now();
  TestPriorityQueue(data.arrays(), &output);
  end_time = clock::now();
  std::cout << "Elapsed time: "
            << std::chrono::duration_cast<
                   std::chrono::duration<double>>(end_time - start_time).count()
            << " seconds \n";
  ValidateOutput(output);

  std::cout << "----\nTesting with customized heap ...\n";
  std::memset(output.data(), 0, sizeof(int) * total_num_elements);
  start_time = clock::now();
  TestCustomizedHeap(data.arrays(), &output);
  end_time = clock::now();
  std::cout << "Elapsed time: "
            << std::chrono::duration_cast<
                   std::chrono::duration<double>>(end_time - start_time).count()
            << " seconds \n";
  ValidateOutput(output);

  std::cout << "----\nTesting with std::sort ...\n";
  std::memset(output.data(), 0, sizeof(int) * total_num_elements);
  start_time = clock::now();
  TestStdSort(data.arrays(), &output);
  end_time = clock::now();
  std::cout << "Elapsed time: "
            << std::chrono::duration_cast<
                   std::chrono::duration<double>>(end_time - start_time).count()
            << " seconds \n";
  ValidateOutput(output);
}
