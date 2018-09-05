xim: A CPU Sia miner
===

Note: Do not use this piece of software if you intend to get rich by mining. 
This is a CPU miner for the SIA cryptocurrency. CPU mining is no longer profitable since the introduction of ASIC mining
for SIA. 
I have written this miner to understand mining and the stratum protocol.  

Dependencies
---
This miner uses the excellent Json library by Niels Lohman ( https://github.com/nlohmann/json/ )

Parameters
---
Parameters are passed with two dashes as follows:
```
./xim --url eu.siamining.com
```

| Parameter | description | example |
|-----------|-------------|---------|
| url | stratum server URL / IP | eu.siamining.com |
| port | stratum server port | 3333 |
| worker | id to identify individual workers, can be empty | worker1 |
| intensity | 2^intensity. i.e. an intensity of 28 scans the range 0...2^28 of values | 28 |
| addr | mining address |99eed232c4749a8bbc505ba7fe9c21fd7261d92438d2a2d4c3069ddc72f4b1cafa21cf0421af |
