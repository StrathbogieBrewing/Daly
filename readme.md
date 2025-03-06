# Daly interface on DNET2

## 

## udev rules
Add the following rules into /etc/udev/rules.d/99-arduino-cli.rules
```
# Create sysmlinks to USB serial devices
SUBSYSTEMS=="usb", ATTRS{interface}=="Dual RS232-HS", KERNELS=="*:1.0", MODE:="0660", GROUP:="dialout", SYMLINK+="ttyESP_jtag"
SUBSYSTEMS=="usb", ATTRS{interface}=="Dual RS232-HS", KERNELS=="*:1.1", MODE:="0660", GROUP:="dialout", SYMLINK+="ttyESP_prog"
```