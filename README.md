# CMondrian

A terminal-based Mondrian-style art generator, creating abstract compositions inspired by Piet Mondrian's work.

![CMondrian Demo](images/example.png)

## Installation

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/scrubmunch/cmondrian.git
cd cmondrian

# Build
mkdir build
cd build
cmake ..
make
sudo make install
```

## Usage

```bash
cmondrian [options]

Options:
  --transparent, -t       Use transparent palette
  --density N, -d N      Set density (2-50, default: 8)
                        Lower numbers create busier art
  --help, -h            Show this help message
```
