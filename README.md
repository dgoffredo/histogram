Automatically Binned Histogram Generator
========================================
Raw data goes in, histogram data comes out.

Bin width is determined by the [Freedman–Diaconis rule][1]. Standard input goes
to standard output, or provide multiple files on the command line to produce
output files `*.hist` corresponding to the input files.

The bin width is calculated over the combined input data, and then each input
is individually histogrammed.

```console
$ ./histogram --help
usage: ./histogram [-h | --help] [--column COLUMN] [--min MIN] [--max MAX] [INPUT_FILE ...]

options:

  -h --help
    Print this message to standard output.

  --column COLUMN
    Read values from the one-based COLUMN of each input line.
    COLUMN is 1 (the first column) by default.

  --min MIN
    Ignore input lines whose value is less than MIN.
    By default, no input lines are ignored.

  --max MAX
    Ignore input lines whose value is greater than MAX.
    By default, no input lines are ignored.
```

[1]: https://en.wikipedia.org/wiki/Histogram#Freedman%E2%80%93Diaconis_rule

