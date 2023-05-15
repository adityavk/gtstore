#/bin/bash

__dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source ${__dir}/run.sh --nodes 1 --rep 1

echo "\n\n\n==========Scenario 1: Basic Single Server GET/PUT==========\n\n"

(
set -x

./bin/test_app --put key1 --val value1
./bin/test_app --get key1
./bin/test_app --put key1 --val value2
./bin/test_app --put key2 --val value3
./bin/test_app --put key3 --val value4
./bin/test_app --get key1
./bin/test_app --get key2
./bin/test_app --get key3

)


echo "\n\n==========Finished Scenario 1: Basic Single Server GET/PUT, restarting servers with next config==========\n\n\n"

source ${__dir}/run.sh --no-build --nodes 5 --rep 3
echo "\n\n\n==========Scenario 2: Basic Multi-Server GET/PUT==========\n\n"

(
set -x

./bin/test_app --put key1 --val value1
./bin/test_app --get key1
./bin/test_app --put key1 --val value2
./bin/test_app --put key2 --val value3
./bin/test_app --put key3 --val value4
./bin/test_app --get key1
./bin/test_app --get key2
./bin/test_app --get key3

)

echo "\n\n==========Finished Scenario 2: Basic Multi-Server GET/PUT, restarting servers with next config==========\n\n\n"

source ${__dir}/run.sh --no-build --nodes 3 --rep 2
echo "\n\n\n==========Scenario 3: Availability through a single node failure==========\n\n"

(
set -x

./bin/test_app --put key1 --val value1
./bin/test_app --put key1 --val newvalue1
./bin/test_app --put key2 --val value2
./bin/test_app --put key3 --val value3
./bin/test_app --put key4 --val value4
./bin/test_app --put key5 --val value5
./bin/test_app --put key6 --val value6
./bin/test_app --put key6 --val newvalue6
./bin/test_app --put key7 --val value7
./bin/test_app --put key8 --val value8
./bin/test_app --put key9 --val value9
./bin/test_app --put key8 --val newvalue8

)


echo "\n\nEnter the pid of server to kill:"
read SERVER_PID
kill -9 "$SERVER_PID"
echo "\n\n"


for i in $(seq 1 9); do
    (
        set -x
        ./bin/test_app --get "key$i"
    )
done

echo "\n\n==========Finished Scenario 3: Availability through a single node failure, restarting servers with next config==========\n\n\n"

source ${__dir}/run.sh --no-build --nodes 7 --rep 3
echo "\n\n\n==========Scenario 4: Availability through multiple node failures==========\n\n"

(
set -x

./bin/test_app --put key1 --val value1
./bin/test_app --put key1 --val newvalue1
./bin/test_app --put key2 --val value2
./bin/test_app --put key3 --val value3
./bin/test_app --put key4 --val value4
./bin/test_app --put key5 --val value5
./bin/test_app --put key6 --val value6
./bin/test_app --put key6 --val newvalue6
./bin/test_app --put key7 --val value7
./bin/test_app --put key8 --val value8
./bin/test_app --put key9 --val value9
./bin/test_app --put key8 --val newvalue8
./bin/test_app --put key10 --val value10
./bin/test_app --put key11 --val value11
./bin/test_app --put key12 --val value12
./bin/test_app --put key13 --val value13
./bin/test_app --put key14 --val value14
./bin/test_app --put key15 --val value15
./bin/test_app --put key16 --val value16
./bin/test_app --put key16 --val newvalue16
./bin/test_app --put key17 --val value17
./bin/test_app --put key18 --val value18
./bin/test_app --put key19 --val value19
./bin/test_app --put key18 --val newvalue18
./bin/test_app --put key20 --val value20
./bin/test_app --put key20 --val newvalue20
./bin/test_app --put key21 --val value21
./bin/test_app --put key21 --val newValue21

)


echo "\n\nEnter the pid of two servers to kill:"
read SERVER_PID
kill -9 "$SERVER_PID"
read SERVER_PID
kill -9 "$SERVER_PID"
echo "\n\n"

for i in $(seq 1 21); do
    (
        set -x
        ./bin/test_app --get "key$i"
    )
done


echo "\n\n==========Finished Scenario 4: Availability through multiple node failures==========\n\n\n"
