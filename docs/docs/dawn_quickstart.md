# Dawn Tutorial

Dawn is the software you will use to interface with your robot. It is a code editor with features that allow you to upload and download code from your robot as well as testing your code through the code runner. Dawn will also display infomation about any connected PiE device for example a motor controller or a limit switch. In this tutorial you will install dawn and create your first student code file using the Student API.

# Getting Started

To install dawn you will need a couple of prerequisites
- a mac, linux, or windows computer
- install of dawn

## Installing Dawn

To install Dawn, go to https://github.com/pioneers/dawn/releases/ and install the latest release for your coresponding operating system. Extract the file using your operating system's unzipping method and then follow the next os specific steps

## Windows

After installing dawn-win32-x64.zip unzip the folder. On Windows, you can do this by right clicking on the ZIP file and selecting "Extract all."

Extracting the ZIP file will create a new folder. Open this folder and find the file "Dawn.exe" (Windows) click on it to start Dawn.

![Dawn.exe](image.png)

## Linux

After installing the dawn-linux-x64.zip unzip to your desiered install directory. Then with the shell open use the commands

```console
user@pc:~$ cd [dawn parent directory]\home\sberkun\Documents\pie\dawn\dawn-packaged\dawn-linux-x64
```

After changing the parent directory to match your own install. This command will put you into the working directory of dawn. From there you can start the program by running the script using the following command

```console
user@pc:~$ ./dawn
```

Assuming you have a linux gui, the program will run displaying dawn

## MacOS

# Dawn Instructions

After installing Dawn you will be presented with Dawn, as well as a interactive guide you can access by clicking on the "Tour button"

![Dawn Starting guide](image-1.png)

The tour will go through all of the special features of dawn including the run modes. 

## Getting Connected

To connect to your robot you will first asdkmlkm