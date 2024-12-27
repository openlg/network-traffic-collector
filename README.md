# Network Traffic Collector(NTC)

网络流量采集器，支持一下功能：

- 使用 pcap 实时采集指定网卡的所有数据包大小
- 直接读取数据包头获取数据长度，无需复制释放数据
- 可根据参数指定将流量推送到服务器
- 支持debug模式查看捕获到的数据流量
- 支持采集服务器 cpu、mem 使用率

# Memory Lost Check

1. 安装 valgrind tools
2. Debug 方式编译 ntc
3. 执行下面的命令检查内存泄露：
```shell
valgrind --tool=memcheck --leak-check=full ./ntc -I5s -s https://cloud.tapdata.net/api/tcm/agent/network_traffic -p 1000
```

