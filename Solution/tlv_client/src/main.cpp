#include "client.hpp"
#include <iostream>
#include <thread>
#include <vector>

template <typename Fn>
void run_concurrent(int concurrency, Fn fn) {
    std::vector<std::thread> threads;
    threads.reserve(concurrency);
    for (int i = 0; i < concurrency; i++) {
        threads.emplace_back(fn);  // each thread runs fn()
    }
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}

int main(int argc, char* argv[]) {
	try {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " <port> <mode> [options]\n";
			std::cerr << "Modes:\n"
					  << "  short                One short-lived client\n"
					  << "  long [duration] [interval]\n"
					  << "                       Long-lived client (default 30s, TLV every 5s)\n"
					  << "  burst [count] [size] Burst of Data TLVs (default 100x16B)\n"
					  << "  oversized            Send TLV with absurd length\n"
					  << "  truncated            Send incomplete TLV\n"
					  << "  unknown              Send TLV with unknown type\n"
					  << "  mixed                Send a mixture of valid/invalid TLVs\n"
					  << "  scenario [short] [long] [buggy_short] [buggy_long]\n"
					  << "                       Mixed workload (defaults: 10 5 5 5)\n";
			return 1;
		}

		std::string host = "127.0.0.1";
		int port = std::atoi(argv[1]);
		std::string mode = argv[2];
		
		auto start = std::chrono::steady_clock::now();   // start timer
		
		// simuates short lived servers
		if (mode == "short") {
			int concurrency = (argc > 3) ? std::atoi(argv[3]) : 1;
			run_concurrent(concurrency, [=]() {
				TLVClient c(host, port);
				c.run_short();
			});
		}
		else if (mode == "long") { // simuates long lived servers by default 30 secs with sending data in every 5 secs.
			int concurrency = (argc > 3) ? std::atoi(argv[3]) : 1;
			int duration    = (argc > 4) ? std::atoi(argv[4]) : 30;
			int interval    = (argc > 5) ? std::atoi(argv[5]) : 5;
			run_concurrent(concurrency, [=]() {
				TLVClient c(host, port);
				c.run_long(duration, interval);
			});
		}
		else if (mode == "burst") {
			int concurrency = (argc > 3) ? std::atoi(argv[3]) : 1;
			int count       = (argc > 4) ? std::atoi(argv[4]) : 100;
			int size        = (argc > 5) ? std::atoi(argv[5]) : 15;
			run_concurrent(concurrency, [=]() {
				TLVClient c(host, port);
				c.run_burst(count, size);
			});
		} else if (mode == "mixed") {
			int concurrency = (argc > 3) ? std::atoi(argv[3]) : 1;
			run_concurrent(concurrency, [=]() {
				TLVClient c(host, port);
				c.run_mixed();
			});
		} else if (mode == "large") {
			int concurrency = (argc > 3) ? std::atoi(argv[3]) : 1;
			run_concurrent(concurrency, [=]() {
				TLVClient c(host, port);
				c.run_large_payload();
			});
		} else if (mode == "scenario") {
			int short_clients = (argc > 3) ? std::atoi(argv[3]) : 10;
			int long_clients = (argc > 4) ? std::atoi(argv[4]) : 5;
			int buggy_short  = (argc > 5) ? std::atoi(argv[5]) : 5;
			int buggy_long   = (argc > 7) ? std::atoi(argv[7]) : 5;

			std::vector<std::thread> threads;
		
			// Short-lived good clients
			for (int i = 0; i < short_clients; i++) {
				threads.emplace_back([=]() {
					TLVClient c(host, port);
					c.run_short();
				});
			}

			// Long-lived good clients
			for (int i = 0; i < long_clients; i++) {
				threads.emplace_back([=]() {
					TLVClient c(host, port);
					c.run_long(30, 5);
				});
			}

			// Buggy short clients
			for (int i = 0; i < buggy_short; i++) {
				threads.emplace_back([=]() {
					TLVClient c(host, port);
					if (i % 3 == 0) c.run_oversized();
					else if (i % 3 == 1) c.run_truncated();
					else c.run_unknown();
				});
			}

			// Buggy long clients
			for (int i = 0; i < buggy_long; i++) {
				threads.emplace_back([=]() {
					TLVClient c(host, port);
					for (int t = 0; t < 60; t += 10) {
						if (t == 0) c.run_unknown();
						else if (t == 10) c.run_truncated();
						else c.run_burst(5, 8); // some noise
						std::this_thread::sleep_for(std::chrono::seconds(10));
					}
					c.run_oversized();
				});
			}

			for (auto& t : threads) t.join();
		} else {
			std::cerr << "Unknown mode: " << mode << "\n";
			return 1;
		}

		auto end = std::chrono::steady_clock::now();     // end timer
		std::chrono::duration<double> elapsed = end - start;
		std::cout << "Execution time: " << elapsed.count() << " seconds\n";
	} catch(std::exception& e) {
		std::cerr<<"Exception in client run loop:"<<e.what()<<std::endl;
	}
	
    return 0;
}
