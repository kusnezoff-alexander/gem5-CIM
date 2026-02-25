# Performance Comparison Summary

## Benchmark Configuration

- **CPU SIMD:** SSE4.1 optimized execution
- **CIM:** Processing-in-Memory (DRAM-based)
- **GPU:** GPU parallel execution (4 compute units)

## Performance Summary

### Simulated Time (Ticks)

| Operation | CPU SIMD | CIM | GPU |
|-----------|----------|-----|-----|
| rowand            | 214,146,998,000 |    433,000 | 698,475,000 |
| rowadd            | 214,146,998,000 |    629,000 | 698,475,000 |
| rowsub            | 214,146,998,000 |    629,000 | 698,475,000 |
| rowmult           | 214,147,405,000 |    629,000 | 698,475,000 |
| rowmin            | 216,056,636,000 |    629,000 | 698,475,000 |
| rowmax            | 214,147,405,000 |    629,000 | 698,475,000 |
| rowequal          | 214,108,474,000 |    629,000 | 698,475,000 |
| rowgreater        | 214,164,962,000 |    629,000 | 698,475,000 |
| rowgreater_equal  | 214,166,523,000 |    629,000 | 698,475,000 |
| rowif_else        | 216,107,841,000 |    629,000 | 698,475,000 |
| rowabs            | 215,691,333,000 | 130,358,000 | 698,475,000 |
| rowbitcount       | 215,798,934,000 |    629,000 | 698,475,000 |

### Speedup vs CPU SIMD

| Operation | CIM | GPU |
|-----------|-----|-----|
| rowand            | 494565.82x |   306.59x |
| rowadd            | 340456.28x |   306.59x |
| rowsub            | 340456.28x |   306.59x |
| rowmult           | 340456.92x |   306.59x |
| rowmin            | 343492.27x |   309.33x |
| rowmax            | 340456.92x |   306.59x |
| rowequal          | 340395.03x |   306.54x |
| rowgreater        | 340484.84x |   306.62x |
| rowgreater_equal  | 340487.32x |   306.62x |
| rowif_else        | 343573.67x |   309.40x |
| rowabs            |  1654.61x |   308.80x |
| rowbitcount       | 343082.57x |   308.96x |

## Key Observations

### Average Speedup

- **CIM:** 325796.88x faster than CPU SIMD
- **GPU:** 307.44x faster than CPU SIMD

## Generated Files

- `performance_comparison.png` - Performance comparison chart
- `speedup_comparison.png` - Speedup comparison chart
- `instruction_comparison.png` - Instruction count comparison
- `detailed_statistics.json` - Complete data in JSON format
- `COMPARISON_SUMMARY.md` - This file
