# Systemd

`systemd` is a Linux software that provides a way to manage which processes run when and how they operate with respect to each other. It is now the most commonly used process manager on Linux distributions. To learn more about how to configure `systemd`, read our [wiki page](https://github.com/pioneers/c-runtime/wiki/Systemd).

## Usage

We use systemd on our Raspberry Pis to automatically start Runtime on robot bootup, and to automatically restart Runtime if a Runtime process somehow fails. **Don't use systemd if this is not for a robot Pi.** To use the service files in this directory, symlink the service files to systemd with

    sudo ln -s $HOME/runtime/systemd/*.service /etc/systemd/system/

Then from the `runtime/systemd` folder, do

    sudo systemctl enable *.service

Now Runtime should automatically start when the Pi boots up!

## Useful commands:

Starting a process
> sudo systemctl start filename.service

Stopping a process
> sudo systemctl stop filename.service

Viewing the status of a process
> sudo systemctl status filename.service

Enabling a process (makes the process start on bootup)
> sudo systemctl enable filename.service

Disabling a process (removes the process from starting on bootup)
> sudo systemctl disable filename.service

Viewing logs that the services emitted:
> journalctl -u filename1 -u filename2
