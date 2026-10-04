# Part 2: measured comparison

The peak median throughput is 2.6 Mops/s at T=1. The predicted peak at one or two threads matches the measured maximum.

Throughput at T=256 is 0.115 times the T=1 value. Across the sampled socket boundary, T=64 to T=96, it changes from 0.7 to 0.4 Mops/s (-42.9%). From T=128 to T=192 it changes from 0.3 to 0.3 Mops/s (+0.0%).

These sampled changes do not establish an abrupt transition or its cause. Use sweep.log to inspect the three-run spread before interpreting small differences. See prediction.md for the hypotheses recorded before the sweep and lscpu.txt and sweep.log for the machine and pinning protocol.
