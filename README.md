# consistent_gossip_ring

##Consistent Hashing

First we should generate the token for nodes. There are two elegant ways:
1. [Generate Token](https://docs.datastax.com/en/cassandra/1.2/cassandra/configuration/configGenTokens_c.html)
```
python -c 'print [str(((2**64 / number_of_tokens) * i) - 2**63) for i in range(number_of_tokens)]'
```
2. Just allocate random virtual tokens which should not equal to those were allocated for other nodes.(Cassandra just use that way) 

We use the following tokens for test
python -c 'print [str(((2**64 / 5) * i) - 2**63) for i in range(5)]'
['-9223372036854775808', '-5534023222112865485', '-1844674407370955162', '1844674407370955161', '5534023222112865484']
