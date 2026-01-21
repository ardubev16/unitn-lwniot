# Low Power Wireless Networking for IoT Labs

This repo contains the lab excercises for the Low Power Wireless Networking for
IoT Labs course. It is setup to quickly and easily build and test with Cooja
simulations your Contiki programs. Works both on Linux and ARM Macs.

## Usage

The only dependency needed is docker (based on your setup you can substitute it
in the `build.sh` script with either `sudo docker` or `podman`), and Java
(tested with Java 21). To get started execute the `./init.sh` script to
bootstrap the contiki and cooja submodles.

To build your project you need to run `./build.sh <relative_path_to_your_project>`.

To run Cooja simulations you need to run `./cooja.sh <relative_path_to_csc_file>`.
