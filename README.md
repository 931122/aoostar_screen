# aoostar screen tool

## prepare

```shell
modprobe k10temp
apt install libfreetype6-dev libcjson-dev fonts-noto-cjk libsensors-dev
```

## build
```shell
cp confing.json.example config.json
make
./tbs
```
