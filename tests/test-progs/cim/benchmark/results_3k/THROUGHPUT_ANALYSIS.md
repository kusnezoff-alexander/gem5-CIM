# Throughput and Energy Efficiency Analysis

## Primitive Operations

| Operation | Variant | GOps/s | GOps/W | Exec Time (s) | Power (W) |
|-----------|---------|---------|---------|---------------|-----------|
| rowand | CPU | 0.000936 | 0.000000 | 0.003205 | 51.00 |

## KNN Workload

| Workload | Variant | GOps/s | GOps/W | Exec Time (s) | Power (W) |
|----------|---------|---------|---------|---------------|-----------|

## Notes

- **GOps/s**: Giga-operations per second (higher is better)
- **GOps/W**: Giga-operations per watt (higher is better)
- **Operation counts**: Primitive ops = 3000 elements, KNN = ~432,000 FLOPs
- **Power model**: DRAM power from gem5 + estimated CPU power (50W @ 4GHz)
- **CPU power**: Conservative estimate, gem5 doesn't model full CPU power consumption