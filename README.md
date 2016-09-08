Credit
======
Originally created by NIIBE Yutaka and published to Github by Joel Dare on 31 Jan 2013.

I forked the code and stated editing as of 8 Sep 2016


Dependency
==========
For Ubuntu:

    sudo apt-get install libusb-dev

Plus the GCC tool chain.


Compiling
=========
To compile the hub-ctrl.c program run the following compile command.

    gcc -o hub-ctrl hub-ctrl.c -lusb -std=c99

That results in an executable binary called `hub-ctrl`.


Usage
=====

List hubs and ports:

    $ sudo ./hub-ctrl
    Hub 0 (Bus 1, Dev 4) - ganged power switching
     ├─ Port  1:  power
     ├─ Port  2:  power
     ├─ Port  3:  power connect enable
     ├─ Port  4:  power
     ├─ Port  5:  power highspeed connect enable
     ├─ Port  6:  power
     └─ Port  7:  power
    Hub 1 (Bus 1, Dev 2) - individual power switching
     ├─ Port  1:  power highspeed connect enable
     ├─ Port  2: 
     ├─ Port  3:  power highspeed connect enable
     ├─ Port  4:  power
     └─ Port  5:  power
    Hub 2 (Bus 1, Dev 1) - ganged power switching
     └─ Port  1:  power highspeed connect enable


Note [`H`ub number] or [`B`us number plus `D`evice number] and `P`ort number of your target port/device. To control its power:

    # use hub and port number to locate device/port
    #   -p 0 = off; 1 = on
    $ sudo ./hub-ctrl -H 0 -P 3 -p 0

or

    # use bus, device and port number to locate device/port
    $ sudo ./hub-ctrl -B 1 -D 4 -P 3 -p 0

A useful command to get more information of USB devices including hierarchy of chained USB hubs is:

    $ lsusb -t
    /:  Bus 01.Port 1: Dev 1, Class=root_hub, Driver=dwc_otg/1p, 480M
        |__ Port 1: Dev 2, If 0, Class=Hub, Driver=hub/5p, 480M
            |__ Port 1: Dev 3, If 0, Class=Vendor Specific Class, Driver=smsc95xx, 480M
            |__ Port 3: Dev 4, If 0, Class=Hub, Driver=hub/7p, 480M
                |__ Port 3: Dev 25, If 0, Class=Human Interface Device, Driver=usbhid, 12M
                |__ Port 3: Dev 25, If 1, Class=Human Interface Device, Driver=usbhid, 12M
                |__ Port 5: Dev 6, If 0, Class=Mass Storage, Driver=usb-storage, 480M


Known Issues
============

- No validation of command-line arguments
- Some USB ports turn back on automatically after being turned off.