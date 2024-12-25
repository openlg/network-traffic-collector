#/bin/bash
apt-get update
apt-get install libcurl4-openssl-dev libpcap-dev libssl-dev -y
apt-get install cmake  -y

cd /ntc
cmake -B ./build -DCMAKE_BUILD_TYPE RELEASE
cmake --build ./build --config RELEASE

sleep 3600