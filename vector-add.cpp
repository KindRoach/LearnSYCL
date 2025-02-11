#include <algorithm>
#include <iostream>
#include <sycl/sycl.hpp>

#include "util.hpp"

using namespace sycl;

constexpr size_t N = 1024 * 1024;

void vector_add(const std::vector<float> &a, const std::vector<float> &b, std::vector<float> &c) {
    for (int i = 0; i < N; ++i) {
        c[i] = a[i] + b[i];
    }
}

void vector_add(
    queue &q,
    buffer<float, 1> &a_buf, buffer<float, 1> &b_buf, buffer<float, 1> &c_buf) {
    // Submit the kernel to the queue
    q.submit([&](handler &h) {
        accessor A{a_buf, h, read_only};
        accessor B{b_buf, h, read_only};
        accessor C{c_buf, h, write_only};

        // BEGIN CODE SNIP
        h.parallel_for(range{N}, [=](id<1> idx) {
            C[idx] = A[idx] + B[idx];
        });
        // END CODE SNIP
    });
}

void test_performance() {
    std::vector<float> a(N), b(N), c(N);

    std::cout << "CPU single core: ";
    benchmark_func([&] { vector_add(a, b, c); });


    // wrap buffer lifecycle
    {
        queue q{cpu_selector_v};
        buffer<float, 1> a_buf{a}, b_buf{b}, c_buf{c};
        std::cout << "CPU SYCL: ";
        benchmark_sycl_kernel([&](queue &q) { vector_add(q, a_buf, b_buf, c_buf); }, q);
    }

    // wrap buffer lifecycle
    {
        queue q{gpu_selector_by_cu};
        buffer<float, 1> a_buf{a}, b_buf{b}, c_buf{c};
        std::cout << "GPU SYCL: ";
        benchmark_sycl_kernel([&](queue &q) { vector_add(q, a_buf, b_buf, c_buf); }, q);
    }
}

void test_acc() {
    // Initialize input and output memory on the host
    std::vector<float> a(N), b(N), c(N);
    std::fill(a.begin(), a.end(), 1);
    std::fill(b.begin(), b.end(), 2);
    std::fill(c.begin(), c.end(), 0);

    queue gpu_q{gpu_selector_by_cu};

    // wrap buffer lifecycle
    {
        buffer<float, 1> a_buf{a}, b_buf{b}, c_buf{c};
        vector_add(gpu_q, a_buf, b_buf, c_buf);
    }

    // Check that all outputs match expected value
    bool passed = std::all_of(c.begin(), c.end(), [](float i) { return i == 3; });
    std::cout << (passed ? "SUCCESS" : "FAILURE") << std::endl;
}

int main() {
    test_acc();
    test_performance();
}
