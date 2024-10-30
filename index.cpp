#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <atomic>

class ReaderWriterLock {
private:
    std::mutex mtx;
    std::condition_variable cv;
    int readers = 0;
    bool writer = false;

public:
    void startRead() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !writer; });
        readers++;
    }

    void endRead() {
        std::unique_lock<std::mutex> lock(mtx);
        if (--readers == 0)
            cv.notify_all();
    }

    void startWrite() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !writer && readers == 0; });
        writer = true;
    }

    void endWrite() {
        std::unique_lock<std::mutex> lock(mtx);
        writer = false;
        cv.notify_all();
    }
};

ReaderWriterLock rwLock;
std::atomic<long long> totalReadWaitTime(0), totalWriteWaitTime(0);
std::mutex coutMtx;

void reader(int id, int readCount) {
    for (int i = 0; i < readCount; ++i) {
        auto startWait = std::chrono::high_resolution_clock::now();
        rwLock.startRead();
        auto endWait = std::chrono::high_resolution_clock::now();

        long long waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(endWait - startWait).count();
        totalReadWaitTime += waitTime;

        {
            std::lock_guard<std::mutex> lock(coutMtx);
            std::cout << "Reader " << id << " waited " << waitTime << " ms, now reading.\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        {
            std::lock_guard<std::mutex> lock(coutMtx);
            std::cout << "Reader " << id << " finished reading.\n";
        }

        rwLock.endRead();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void writer(int id, int writeCount) {
    for (int i = 0; i < writeCount; ++i) {
        auto startWait = std::chrono::high_resolution_clock::now();
        rwLock.startWrite();
        auto endWait = std::chrono::high_resolution_clock::now();

        long long waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(endWait - startWait).count();
        totalWriteWaitTime += waitTime;

        {
            std::lock_guard<std::mutex> lock(coutMtx);
            std::cout << "Writer " << id << " waited " << waitTime << " ms, now writing.\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        {
            std::lock_guard<std::mutex> lock(coutMtx);
            std::cout << "Writer " << id << " finished writing.\n";
        }

        rwLock.endWrite();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void runTest(int numReaders, int numWriters, int readCount = 3, int writeCount = 3) {
    totalReadWaitTime = 0;
    totalWriteWaitTime = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < numReaders; ++i)
        threads.emplace_back(reader, i + 1, readCount);
    for (int i = 0; i < numWriters; ++i)
        threads.emplace_back(writer, i + 1, writeCount);

    for (auto &th : threads)
        th.join();

    long long avgReadWait = totalReadWaitTime / (numReaders * readCount);
    long long avgWriteWait = totalWriteWaitTime / (numWriters * writeCount);

    std::cout << "Readers: " << numReaders << ", Writers: " << numWriters << "\n";
    std::cout << "Average reader wait time: " << avgReadWait << " ms\n";
    std::cout << "Average writer wait time: " << avgWriteWait << " ms\n\n";
}

int main() {
    std::cout << "Test case 1: More readers than writers\n";
    runTest(5, 2);

    std::cout << "Test case 2: Equal readers and writers\n";
    runTest(3, 3);

    std::cout << "Test case 3: More writers than readers\n";
    runTest(2, 5);

    return 0;
}
