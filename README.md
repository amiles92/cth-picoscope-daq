### Prerequisites

* PyBind11 is used for python interfacing
  Install in Ubuntu:
  ```sh
  apt install pybind11-dev
  ```
  Confirm it works by running:
  ```sh
  pybind11-config
  ```
* gcc-11 for PyBind11
* PicoScope libraries (only made for ps6000 for the moment)
  ```sh
  sudo apt install libps6000
  ```
* Python3 (3.10 might specifically be required but you can probably just change the make line to fix this)

### Installation

1. Run make
  ```
  make
  ```
  Hope that worked

