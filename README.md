Automatically Binned Histogram Generator
========================================
Raw data goes in, histogram data comes out.

Bin width is determined by the [Freedman–Diaconis rule][1]. Standard input goes
to standard output, or provide multiple files on the command line to produce
output files `*.hist` corresponding to the input files.

The bin width is calculated over the combined input data, and then each input
is individually histogrammed.

Set the one-based input data column with the `--column COLUMN` option. It
defaults to 1 (the first column).

```console
$ ./histogram --help
usage: ./histogram [-h | --help] [(-c | --column) COLUMN] [INPUT_FILE ...]
```

[1]: https://en.wikipedia.org/wiki/Histogram#Freedman%E2%80%93Diaconis_rule
