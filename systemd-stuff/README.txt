systemd provides a way to manage which processes run when and how they operate with respect to each other. systemd can only be used on Linux.

1) Create an empty unit file (filename.service) for each process you want to manage and place it in /etc/systemd/system. This file describes how systemd should handle each process.
> cd /etc/systemd/system
> sudo touch filename.service
> sudo chmod 664 filename.service

2) Use your favorite editor to edit each unit file
> sudo vim filename.service

3) Follow the template unit file (should be in the same directory as this README.txt file). Each unit file is split by sections, which start with [SectionName] on some line and have some settings on the lines after. Here is a description for some settings (do not copy the following lines).

[Unit]
Description=The description for the process being managed.
BindsTo=A list of unit files for other processes that this process should be bound to. For example, if it's set to "a.service b.service c.service", then when one of the processes managed by a/b/c.service starts/restarts/stops, then the process managed by this unit file will also start/restart/stop. This setting is one-directional, so if the process managed by this unit file starts/restarts/stops, then it won't affect the processes managed by a/b/c.service

[Service]
ExecStart=Command for starting the process. Only use absolute paths, no relative paths. For executables (which includes the output of compiling c programs), the command is just the absolute path to the executable. If you have something like a python script that isn't executable, the command would be something like "/path/to/python /location/of/python/script.py"
Restart=Condition for restarting the process. "on-failure" means any non-zero exit status or unclean signal.

[Install]
WantedBy=A list of unit files that "want" this process to start. Basically just set it to "multi-user.target" since it's unlikely this setting will be set to anything else. Effectively, it will cause this process to start on startup.

4) Run the following command for the unit file(s) that have the Install section.
> sudo systemctl enable filename.service

5) Reboot to see the process(es) start automatically
> sudo reboot

Useful commands:
Starting a process
> sudo systemctl start filename.service
Stopping a process
> sudo systemctl stop filename.service
Viewing the status of a process
> sudo systemctl status filename.service
Enabling a process
> sudo systemctl enable filename.service
Disabling a process
> sudo systemctl disable filename.service

References:
Settings for the Unit/Install sections of unit files
https://www.freedesktop.org/software/systemd/man/systemd.unit.html

Settings for the Service section of the unit files
https://www.freedesktop.org/software/systemd/man/systemd.service.html
