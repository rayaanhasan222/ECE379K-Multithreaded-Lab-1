| Threads | Operations | Mops/s | Cycles/op | Instructions/op | IPC | L1 misses/op | LLC misses/op | HITM/op | Context switches/op |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 70,477,824 | 2.3 | 1399.07 | 334.63 | 0.23918 | 24.0001 | 0.267381 | 0.000163115 | 5.67554e-08 |
| 8 | 23,984,128 | 0.8 | 17964.9 | 4727.29 | 0.26314 | 131.345 | 27.659 | 11.2422 | 0.502106 |
| 28 | 23,953,408 | 0.8 | 95696.7 | 11402.9 | 0.119156 | 159.532 | 36.2483 | 16.0461 | 0.543472 |

CoarseMap on Frontera. Event counts are divided by the full reported operation count; IPC is instructions/cycles. LLC misses use perf's cache-misses event, and HITM uses mem_load_l3_hit_retired.xsnp_hitm. Counters start after the configured delay, so document the small difference between counting and benchmark windows. Cycles/op sum cycles across threads and are not operation wall-clock latency.
