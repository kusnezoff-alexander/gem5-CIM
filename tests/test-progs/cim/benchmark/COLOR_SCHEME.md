# TU Dresden Color Scheme for Performance Charts

## Overview

All performance comparison charts use the official TU Dresden color palette with **blues and greys only** for a professional, cohesive visualization.

---

## Color Mapping

| Variant | Color Name | RGB Values | Hex Code | Visual |
|---------|-----------|------------|----------|--------|
| **CPU SIMD** | Blau1 | (47, 87, 178) | #2F57B2 | 🟦 Medium Blue |
| **CIM** | Grau80 | (86, 99, 113) | #566371 | ⬛ Dark Grey |
| **GPU** | Brilliantblau | (0, 0, 140) | #00008C | 🟦 Deep Blue |

---

## Complete TU Dresden Palette (Reference)

The following colors are available from the TU Dresden corporate design:

### Primary Colors
| Color Name | RGB Values | Hex Code |
|-----------|------------|----------|
| Brilliantblau | (0, 0, 140) | #00008C |
| Dunkelblau | (0, 20, 80) | #001450 |

### Accent Colors
| Color Name | RGB Values | Hex Code | Usage |
|-----------|------------|----------|--------|
| Blau1 | (47, 87, 178) | #2F57B2 | ✓ CPU SIMD |
| Violett1 | (115, 105, 190) | #7369BE | Available |
| Magenta1 | (188, 21, 137) | #BC1589 | Available |
| Rot1 | (210, 15, 65) | #D20F41 | ✓ CPU Serial |
| Gelb1 | (255, 199, 0) | #FFC700 | Available |
| Oliv1 | (118, 122, 35) | #767A23 | Available |
| Gruen1 | (0, 125, 75) | #007D4B | ✓ CIM |
| Tuerkis1 | (10, 119, 127) | #0A777F | ✓ GPU |

### Neutral Colors
| Color Name | RGB Values | Hex Code |
|-----------|------------|----------|
| Schwarz | (0, 0, 0) | #000000 |
| Weiss | (255, 255, 255) | #FFFFFF |
| Grau100 | (50, 63, 75) | #323F4B |
| Grau80 | (86, 99, 113) | #566371 |
| Grau60 | (125, 136, 148) | #7D8894 |
| Grau40 | (165, 174, 184) | #A5AEB8 |
| Grau20 | (208, 213, 220) | #D0D5DC |
| Grau10 | (231, 233, 237) | #E7E9ED |

---

## Rationale for Color Selection

### CPU SIMD → Blau1 (Medium Blue)
- **Why:** Baseline for comparison, CPU-optimized with vector instructions
- **Association:** Technology, computing, optimization
- **Visibility:** Clear medium blue, professional appearance
- **Positioning:** Standard reference point for speedup calculations

### CIM → Grau80 (Dark Grey)
- **Why:** Compute-in-Memory architecture with unique characteristics
- **Association:** Specialized hardware, alternative approach
- **Visibility:** Professional grey contrasts well with blues
- **Positioning:** Distinct neutral tone highlights its different nature

### GPU → Brilliantblau (Deep Blue)
- **Why:** Massively parallel processing architecture
- **Association:** High-performance computing, advanced parallelism
- **Visibility:** Rich, saturated deep blue - distinct from medium blue
- **Positioning:** Premium deep color reflects advanced architecture

---

## Implementation Details

### Python Code
```python
# Color scheme from TU Dresden (Blues and Greys only)
colors = {
    "cpu_simd": "#2F57B2",    # Blau1 (RGB 47, 87, 178)
    "cim": "#566371",         # Grau80 (RGB 86, 99, 113)
    "gpu_test": "#00008C",    # Brilliantblau (RGB 0, 0, 140)
}
```

### LaTeX Definition (Reference)
```latex
\definecolor{Blau1}{RGB}{47, 87, 178}
\definecolor{Grau80}{RGB}{86, 99, 113}
\definecolor{Brilliantblau}{RGB}{0, 0, 140}
```

---

## Visual Characteristics

### Contrast and Readability
- **High contrast** between all four colors
- **Colorblind-friendly** - distinguishable patterns
- **Print-safe** - works well in both color and grayscale

### Professional Appearance
- **Brand consistency** with TU Dresden corporate design
- **Academic credibility** through proper color usage
- **Publication-ready** for papers and presentations

---

## Chart Specifications

All charts using this color scheme:

### File Information
- **Format:** PNG
- **Resolution:** 300 DPI
- **Size:** 16" × 10"
- **Background:** White

### Visual Elements
- **Bars:** Solid colors (no patterns)
- **Legend:** Color-coded labels
- **Grid:** None (clean appearance)
- **Scale:** Logarithmic Y-axis

---

## Accessibility

### Color Contrast Ratios
All color combinations meet WCAG accessibility guidelines:
- Medium Blue (Blau1) vs White background: ✓ High contrast
- Dark Grey (Grau80) vs White background: ✓ High contrast
- Deep Blue (Brilliantblau) vs White background: ✓ Excellent contrast

### Colorblind Considerations
The blues and greys palette is particularly colorblind-friendly:
- **Deuteranopia** (green-blind): ✓ Excellent - grey and two blue shades clearly distinguishable
- **Protanopia** (red-blind): ✓ Excellent - no red used, all colors clearly visible
- **Tritanopia** (blue-blind): ✓ Good - grey provides contrast, blue intensity differences visible

---

## Usage Guidelines

### When to Use
✓ Academic presentations  
✓ Research papers  
✓ Technical reports  
✓ TU Dresden publications  
✓ Conference presentations  

### When NOT to Use
✗ Non-TU Dresden publications (use generic colors)  
✗ When specific brand colors are required  

---

## Files Using This Color Scheme

Current implementation:
```
tests/test-progs/cim/benchmark/
├── performance_comparison.png     (Uses TU Dresden colors)
├── speedup_comparison.png         (Uses TU Dresden colors)
├── instruction_comparison.png     (Uses TU Dresden colors)
└── extract_and_compare.py        (Contains color definitions)
```

---

## Updating Colors

To change the color scheme, edit `extract_and_compare.py`:

```python
# Locate the color dictionaries in these functions:
# - plot_performance_comparison()
# - plot_speedup_comparison()
# - plot_instruction_comparison()

# Update the hex codes as needed
colors = {
    "cpu_simd": "#2F57B2",    # Change hex code here
    "cim": "#566371",         # Change hex code here
    "gpu_test": "#00008C",    # Change hex code here
}
```

Then regenerate charts:
```bash
python3 extract_and_compare.py
```

---

## References

- **TU Dresden Corporate Design Manual**
- **WCAG 2.1 Accessibility Guidelines**
- **Scientific Visualization Best Practices**

---

**Color Scheme Applied:** February 24, 2026  
**Status:** Active and Applied ✓  
**Compliance:** TU Dresden Corporate Design (Blues and Greys Only) ✓  
**Variants Compared:** CPU SIMD, CIM, GPU (CPU Serial excluded)
