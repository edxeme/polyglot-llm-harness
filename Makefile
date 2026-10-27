.PHONY: java rust cpp smoke clean

java:
	javac -d java/build $$(find java/src -name '*.java')

rust:
	cd rust && cargo build --release

cpp:
	cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release && cmake --build cpp/build

smoke: java rust cpp
	java -cp java/build dev.edxeme.lmh.Main run merge-intervals
	java -cp java/build dev.edxeme.lmh.Main run lru-cache
	java -cp java/build dev.edxeme.lmh.Main run rate-limiter
	rust/target/release/lmh run merge-intervals
	rust/target/release/lmh run lru-cache
	rust/target/release/lmh run rate-limiter
	cpp/build/lmh run merge-intervals
	cpp/build/lmh run lru-cache
	cpp/build/lmh run rate-limiter

clean:
	rm -rf java/build rust/target cpp/build
