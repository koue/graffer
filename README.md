Graffer is a small utility that collects numeric values from external programs
and produces graphs like mrtg, pfstat or alike.

## Installation

### Requirements
* png

```
make && make install
```

## Usage

```
$ graffer -q -c etc/graffer.conf.example
$ graffer -p -c etc/graffer.conf.example
$ graffer -g '7:from 24 hours to now' -c etc/graffer.conf.example
```
