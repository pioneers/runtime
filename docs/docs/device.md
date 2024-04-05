# Lowcar Device reference

Lowcar is the name given to all of the code in the repo that runs on an Arduino microcontroller, which controls the various devices. Most of your teleoperated code will designed to interface with these lowcar devices. These devices include BatteryBuzzer, KoalaBear (motor controller), LimitSwitch, LineFollower, and ServoControl. In this tutorial, you will find in-depth explanations of each device and how to control them.

For more detailed information and descriptions on the Lowcar devices please refer to the [runtime wiki](https://github.com/pioneers/runtime/wiki/Lowcar)

# More Background on the infomration

in each device page, the device information will be separated into three parts, a photo of the device, the device's parameters, a general description of it's intended usage and other possible applications.

## Limit Switch
![Limit Switch](https://raw.githubusercontent.com/pioneers/runtime-docs/master/device-pics/LimitSwitch.JPG)

### Parameters
| Name    | Data type | Readable| Writeable |
|---------|-----------|---------|-----------|
| switch0 | BOOL      | TRUE    | FALSE     |
| switch1 | BOOL      | TRUE    | FALSE     |
| switch2 | BOOL      | True    | FALSE     |



A limit switch is a device wired with three switches. These switches, `switch0`, `switch1`, AND `switch2` each have a metal actuator attached to them. The original purpose of a limit switch is to limit an object from going past a certain endpoint. However, it's use isn't limited to limiting the range of an object, for example using it to detect the presence of an object in a certain place or not. 

