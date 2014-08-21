ulimit -c unlimited
export LD_LIBRARY_PATH=../lib/$LD_LIBRARY_PATH
./HallServer ../conf/config.xml -p 4700 -s 1
./HallServer ../conf/config.xml -p 4701 -s 2

