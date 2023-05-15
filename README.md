# Project 4 - GTStore

## Build steps
1. `cd <root>`
2. Run `./build.sh [--clean]` to build the project.
3. To spawn manager and n storage nodes, run `./run.sh [--no-build] [--clean] [--help] --nodes <n> --rep <k>`. This also rebuilds the project, unless `--no-build` argument is specified.
4. To use the demo test app after running the manager and storage nodes, run:
    * `./bin/test_app --get <key>` to get the value for a key from the store
    * `./bin/test_app --put <key> --val <value>` to set/modify the value for a key in the store
5. To run all demo tests in one go, just run `./test.sh` without any build or run commands. 
6. To run the performance test app, run
    * `./bin/perf_test_app --lb` to get results for load-balance test
    * `./bin/perf_test_app --throughput` to get results for throughput test
7. To clean, run `rm -rf cmake bin`.
