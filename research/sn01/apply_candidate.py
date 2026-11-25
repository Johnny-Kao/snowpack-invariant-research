from pathlib import Path
p=Path("snowpack/DataClasses.cc")
s=p.read_text()
a=s.index("\t// Iterate from the top down, stopping at soil or the end of the snowpack", s.index("double SnowStation::averageFromTop"))
b=s.index("\n}",a)
assert "ii > SoilNode" in s[a:b] and "return sum / averagingDepth;" in s[a:b]
replacement="""\tfor (size_t ii = Edata.size(); ii > SoilNode && cumulativeDepth < averagingDepth; ) {
\t\tconst auto& elem = Edata[--ii];
\t\tconst double weight = std::min(elem.L, averagingDepth - cumulativeDepth);
\t\tsum += weight * Elementproperty(elem);
\t\tcumulativeDepth += weight;
\t}
\treturn cumulativeDepth > 0. ? sum / cumulativeDepth : 0.;"""
p.write_text(s[:a]+replacement+s[b:])
